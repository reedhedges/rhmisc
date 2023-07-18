

// Exersize to provide iterator interface (can be used as range) to pieces of data obtained from somewhere on demand until some end condition is reached.  (In this example, we just read lines from a file until EOF.) 
//
// This is just an exersize to implement the iterator and other stuff required.  Reading from a file is a stand-in for some
// other kind of IO or IO-like operation which we lazily use to receive each piece of data. (E.g. instead of getline() we could
// use some other call into some library or system.)   Reading data directly from an actual file can more easily done with std::getline() in a loop or 
// std::istream_view or std::istream_iterator with a new class that uses getline() to read lines (rather than using std::string which causes 
// istream_view or iterator to return space-separated "words":  (Would be nice if its delimiter was customizable, but haven't found a way yet.) 
//
//      template <typename CharT = char, CharT delim = '\n'>
//      struct line 
//      {
//        std::string s;
//        friend std::istream& operator>>(std::istream& is, line<CharT, delim>& l) 
//        {
//          return std::getline(is, l.s, delim);
//        }
//        operator std::string_view () const noexcept { return s; }
//        friend std::ostream& operator<<(std::ostream& os, line<CharT, delim>& l)
//        {
//          os << l.s; return os;
//        }
//      };
//
//      std::ifstream is("file.txt");
//      for(const auto& line : std::ranges::istream_view<line>(is))
//      {
//        std::cout << line << '\n';
//      }
//
//
//  This is a more standard iterator implementation, and is generally cleaner.  The iterator class is lighter weight and
//  more consistent with standard iterators.  More methods are const and/or noexcept.
//
//  For an alternative design, however, see read_file_lines_as_range_1.cc



// for strerrordesc_np() (GNU) or strerror().:
//#define _GNU_SOURCE 1
//#define HAVE_STRERRORDESC_NP 1
#include <cstring>

#ifdef HAVE_STRERRORDESC_NP
#warning using GNU specific strerrordesc_np() instead of strerror()...
                #define STRERROR(code) (strerrordesc_np(code))
#else
                #define STRERROR(code) (strerror(code))
#endif

#include <filesystem>
#include <string>
#include <string_view>
#include <iterator>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>

#include <ranges>
#include "fmt/format.h"






class FileChunkReader;



// You can use any class (type) as a sentinel, but since all we need is return it
// from end() and then compare it against FileChunkIterators to test for end, we
// can use an empty class. The standard library provides one called default_sentinel_t so
// we will use that below instead of this:
//class FileChunkIteratorSentinel
//{
//};



// Note, delimiter character is not removed when chunk is read from file.
class FileChunkIterator
{
public:
    //using iterator_category = std::forward_iterator_tag; // does not (yet?) meet the requirements for forward iterator.
    using iterator_category = std::input_iterator_tag;
    using difference_type = ssize_t;
    using value_type = std::string_view;
    using reference = value_type&;
    using pointer = value_type*;

private:
    FileChunkReader* reader = nullptr;
    difference_type counter = 0;

public:

    // note, reads first line, may throw
    explicit FileChunkIterator(FileChunkReader* fr_) :
        reader(fr_)
    {
      read_line();
    }
    
private:
    // may throw system_error on error
    void read_line(); // definition is below, after definition of FileChunkReader.

    void advance()
    {
      read_line();
      ++counter;
    }

    std::string_view get_line() const noexcept; // definition below after definition of FileChunkReader.

public:

    // Warning: The returned string_view will become invalid when this iterator is advanced. Copy it to std::string or similar
    // if saving or using it beyond one iteration (e.g. in a range view).
    const value_type operator*() const noexcept
    {
      return get_line();
    }

    FileChunkIterator& operator++() // not noexcept, may throw. Not const.
    {
      advance();
      return *this;
    }
  
    // post increment:
    FileChunkIterator operator++(int) // not noexcept, may throw. Not const.
    {
      FileChunkIterator selfcopy = *this;
      advance();
      return selfcopy;
    }

    friend bool operator==(const FileChunkIterator& lhs, const FileChunkIterator& rhs) noexcept
    {
      // good comparison?
      return (lhs.reader == rhs.reader) && (lhs.counter == rhs.counter);
    }

    bool at_end() const noexcept; // definition below after definition of FileChunkReader

    // This is how end of the iteration is detected (e.g. 'if(i == std::end(reader))': The FileChunkIterator 'i' is the same as 'end()' (which returns a std::default_sentinel_t) if it is at EOF.
    friend bool operator==(const FileChunkIterator& i, const std::default_sentinel_t&) noexcept
    {
      return i.at_end();
    }

    // needed? correct?
    //friend difference_type operator-(const FileChunkIterator& lhs, const FileChunkIterator& rhs) noexcept
    //{
    //  // todo error if open on different files?
    //  return(lhs.counter - rhs.counter);
    //}


    // TODO could implement += by repeated advance(), or scanning file characters for delimiter without reading into buffer.

    // TODO could maybe implement -- by seeking backwards scanning for delimiter.

};







// Opens the specified file, and provides begin() and end() FileChunkIterators.  These
// iterators use read_line() to read the next chunk delimited by the given delimiter.
// File is closed when this object is destroyed.
class FileChunkReader
{
private:
  // FILE* and getdelim() are used for file access.
  FILE* fp = nullptr;
  int delimiter = 0;
  char *buf = nullptr;
  size_t bufsize = 0;
  size_t bufstrlen = 0;

public:

  FileChunkReader(std::filesystem::path path, int delim_) : delimiter(delim_)
  {
    fp = fopen(path.c_str(), "r");
    if(!fp)
    {
        std::string msg(STRERROR(errno));
        msg += ": \"";
        msg += path.c_str();
        msg += "\"";
        throw std::system_error(errno, std::system_category(), msg);
    }
  }

  FileChunkReader(FileChunkReader&& old) noexcept
  {
    fp = old.fp;
    delimiter = old.delimiter;
    old.fp = nullptr;
  }

  FileChunkReader& operator=(FileChunkReader&& old) noexcept
  {
    fp = old.fp;
    delimiter = old.delimiter;
    old.fp = nullptr;
    return *this;
  }

  ~FileChunkReader() noexcept
  {
    if(buf)
      free(buf);
    if(fp)
    {
      int err = fclose(fp);
      if(err != 0) [[unlikely]]
      {
          fmt::print("FileChunkReader: Warning: error {} closing file: {}\n", errno, STRERROR(errno));
          // throw std::system_error(errno, std::system_category(), STRERROR(errno));
          assert(err == 0);
      }
    }
  }
    

  FileChunkIterator begin() noexcept
  {
    return FileChunkIterator(this);
  }

  std::default_sentinel_t end() noexcept
  {
    // todo should we also pass in fd so that FileChunkIterator::operator==() can check that too?
    return std::default_sentinel;  // this is just a global constant instance of default_sentinel_t.
  }

  // no const iterators because any advance of iterator is mutating operation on file reader.

  bool at_end() const noexcept // could rename "invalid" or something. could be private except for friend FileChunkIterator.
  {
    return feof(fp) || ferror(fp);
  }

  // Returns a string_view of the current line buffer.
  // Note: Use the returned string_view only until the next call to read_line() -- after read_line() it will 
  // be an incorrect view of the next buffer (or it may be an invalid view to deallocated memory.)
  std::string_view get_line() const noexcept
  {
    return std::string_view{buf, bufstrlen};
  }

  // read next line into buffer (buf/bufstrlen)
  void read_line()
  {
    // todo error if file at eof. (throw?)
    ssize_t r = getdelim(&buf, &bufsize, delimiter, fp);
    if(r < 0) [[unlikely]]
    {
      // causes operator* to return a null string view (or, should operator* throw exception after eof/error?):
      if(buf) free(buf);
      buf = nullptr;
      bufsize = 0;
      bufstrlen = 0;

      if(feof(fp))
      {
        //eof = true;
      }
      else
      {
        throw std::system_error(errno, std::system_category(), STRERROR(errno));
      }
    }
    else
    {
      bufstrlen = (size_t)r;
    }
    //fmt::print("getdelim returned {}, bufstrlen={}\n", r, bufstrlen);
  }


};



void FileChunkIterator::read_line()
{
  reader->read_line();
}

std::string_view FileChunkIterator::get_line() const noexcept
{
      // Currently the first line is read by the constructor. this could instead be done here. But would that
      // violate 'const'? Or is it ok if we are just calling reader->read_line() (which, however, is not const,
      // i.e. we are not propagating const in this case.) 
  return reader->get_line();
}

bool FileChunkIterator::at_end() const noexcept
{
  return reader->at_end();
}

// add overloads for std::begin() and std::end():
namespace std 
{
  FileChunkIterator begin(FileChunkReader& r) noexcept { return r.begin(); }
  std::default_sentinel_t end(FileChunkReader& r) noexcept { return r.end(); }
};





void test_iterate()
{
    fmt::print("--> test_iterate reading lines from \"testfile.txt\"...\n");
    FileChunkReader fr("testfile.txt", '\n');
    static_assert(std::input_iterator<FileChunkIterator>);
    // not a  forward iterator :
    //static_assert(std::forward_iterator<FileChunkIterator>);
    for (auto line : fr)
    {
      fmt::print("\t> read line: '{}', length={}\n", line, line.size());
    }
    fmt::print("...done.\n");
}




void test_range()
{
    fmt::print("--> test_range reading lines from \"testfile.txt\"...\n");
    FileChunkReader fr("testfile.txt", '\n');
    // strip newlines from ends, filter out short words:
    auto longwords = std::ranges::transform_view(fr,
        [](const std::string_view s) -> std::string_view { 
          if (s.ends_with("\n"))
            return std::string_view(s.begin(), s.end()-1);
          else
            return s;
        })
      | std::ranges::views::filter([](const auto s){ return (s.length() > 3); });
    for (auto line : longwords)
    {
      fmt::print("\t>3 characters: '{}'\n", line);
    }
    fmt::print("...done.\n");
}





int main()
{
  test_iterate();
  test_range();
  return 0;
}

