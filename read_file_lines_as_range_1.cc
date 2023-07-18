

// Exersize to provide iterator interface (can be used as range) to pieces of data obtained from somewhere on demand until some end condition is reached.  (In this example, we just read lines from a file until EOF.) 
// Build with:
//      c++ -std=c++20 -Wall -Wextra -o read_file_lines_as_range.cc
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
//  Currently multiple FileChunkIterators can separately iterate through the data of the same open file (open file is represented by
//  FileChunkReader which also provides new begin and end FileChunkIterators; each iterator contains its own FILE* stream pointer
//  and stores the last read file line in its own buffer (which may not be the best design for an iterator.) 
//  Each FileChunkIterator when advanced reads the next chunk into its own buffer, which can be accessed with
//  operator*.  
//
//  An alternative design would be instead for FileChunkReader to be the only reader of the file, reading new data on request from
//  any iterator, and providing data from its own buffer. This is probably a cleaner implementation.  For this implementation, see read_file_lines_as_range_2.cc.
//


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
    FILE* fp = nullptr;
    int delimiter = 0;
    char *buf = nullptr; // allocated by getdelim(), but we must free in destructor
    size_t bufsize = 0;
    size_t bufstrlen = 0;
    difference_type counter = 0;

public:

    // note, reads first line, may throw
    FileChunkIterator(FILE* fp_, int delim_) :
        fp(fp_), delimiter(delim_)
    {
      read_line();
    }
    
    // copy (also copies buffer contents):

    FileChunkIterator(const FileChunkIterator& other)  :
      fp(other.fp),
      delimiter(other.delimiter),
      bufsize(other.bufsize),
      bufstrlen(other.bufstrlen),
      counter(other.counter)
    {
      buf = (char*) malloc(bufsize * sizeof(char));
      memcpy(buf, other.buf, bufsize);
    }

    FileChunkIterator& operator=(const FileChunkIterator& other)
    {
        // todo check that this is right:
        if(&other != this) 
            FileChunkIterator(std::forward<const FileChunkIterator&>(other)); 
        return *this; 
    }

    // move:

    FileChunkIterator(FileChunkIterator&& old) noexcept :
        fp(old.fp),
        delimiter(old.delimiter),
        buf(old.buf),
        bufsize(old.bufsize),
        bufstrlen(old.bufstrlen)
    {
      old.buf = nullptr;
      old.bufsize = 0;
      old.bufstrlen = 0;
    }

    FileChunkIterator& operator=(FileChunkIterator&& old) noexcept 
    { 
        if(&old != this) 
            FileChunkIterator(std::forward<FileChunkIterator&&>(old)); 
        return *this; 
    }

    ~FileChunkIterator() noexcept
    {
      if(buf)
        free(buf);
    }


private:
    // read next line into buffer (buf/bufstrlen)
    // todo move to FileChunkReader
    void read_line()
    {
      // todo error if file at eof. (throw?)
      ssize_t r = getdelim(&buf, &bufsize, delimiter, fp);
      if(r < 0) [[unlikely]]
      {
        // causes operator* to return a null string view. (alternatively, should operator* throw exception after error/eof?)
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
          printf("Error reading file: %d %s\n", errno, STRERROR(errno));
          throw std::system_error(errno, std::system_category(), STRERROR(errno));
        }

      }
      else
      {
        bufstrlen = (size_t)r;
      }
    }

    void advance()
    {
      // todo should this work move to operator*? (would also need to check counter vs. increment request)
      read_line();
      ++counter;
    }

public:

    // Warning: The returned string_view will become invalid when this iterator is advanced. Copy it to std::string or similar
    // if saving or using it beyond one iteration (e.g. in a range view).
    const value_type operator*() const noexcept
    { 
      // Currently the first line is read by the constructor. this could instead be done here. But would that
      // violate 'const'? Or is it ok if a 'const FileChunkIterator' can't be incremented with ++, but still
      // does internal stuff like file IO and updating 'counter'?
      return std::string_view(buf, bufstrlen);
    }

    FileChunkIterator& operator++() // not noexcept, may throw
    {
      advance();
      return *this;
    }
  
    // post increment:
    FileChunkIterator operator++(int) // not noexcept, may throw
    {
      FileChunkIterator selfcopy = *this;
      advance();
      return selfcopy;
    }

    friend bool operator==(const FileChunkIterator& lhs, const FileChunkIterator& rhs) noexcept
    {
      // good comparison?
      return (lhs.fp == rhs.fp) && (lhs.counter == rhs.counter);
    }

    bool at_end() const noexcept
    {
      return feof(fp) || ferror(fp);
    }

    // This is how end of the iteration is detected (e.g. 'if(i == std::end(reader))': The FileChunkIterator 'i' is the same as 'end()' (which returns a std::default_sentinel_t) if it is at EOF.
    friend bool operator==(const FileChunkIterator& i, const std::default_sentinel_t&) noexcept
    {
      return i.at_end();
    }

    // needed? correct?
    //friend difference_type operator-(const FileChunkIterator& lhs, const FileChunkIterator& rhs) noexcept
    //{
    //  // todo error if open ot different files?
    //  return(lhs.counter - rhs.counter);
    //}


    // TODO could implement += by repeated advance(), or scanning file characters for delimiter without reading into buffer.

    // TODO could maybe implement -- by seeking backwards scanning for delimiter.

};







// Provides begin() and end() FileChunkIterators.
// It opens the file, and creates new FILE* streams when creating new iterators.
// File is closed when this object is destroyed.
class FileChunkReader
{
  int fd = -1;
  int delimiter = 0;
public:
  FileChunkReader(std::filesystem::path path, int delim_) : delimiter(delim_)
  {
    fd = open(path.c_str(), O_RDONLY);
    if(fd < 0)
    {
        std::string msg(STRERROR(errno));
        msg += ": \"";
        msg += path.c_str();
        msg += "\"";
        throw std::system_error(errno, std::system_category(), msg);
    }
  }

  FileChunkReader(FileChunkReader&& old)
  {
    fd = old.fd;
    delimiter = old.delimiter;
    old.fd = -1;
  }

  FileChunkReader& operator=(FileChunkReader&& old)
  {
    fd = old.fd;
    delimiter = old.delimiter;
    old.fd = -1;
    return *this;
  }

  ~FileChunkReader() noexcept
  {
    if(fd != -1)
    {
      int err = close(fd);
      if(err != 0)
      {
          fmt::print("FileChunkReader: Warning: error {} closing file descriptor {}: {}\n", errno, fd, STRERROR(errno));
          // throw std::system_error(errno, std::system_category(), STRERROR(errno));
          assert(err == 0);
      }
    }
  }
    

  FileChunkIterator begin()
  {
    // create a new FILE* from our file descriptor and set position to beginning of file. (This should not affect other FILE* pointers used in any other iterators.)
    FILE *fp = fdopen(fd, "r");
    clearerr(fp);
    rewind(fp);
    return FileChunkIterator(fp, delimiter);
  }

  std::default_sentinel_t end() const noexcept
  {
    // todo should we also pass in fd so that FileChunkIterator::operator==() can check that too?
    return std::default_sentinel;  // this is just a global constant instance of default_sentinel_t.
  }

};

// add overloads for std::begin() and std::end():
namespace std 
{
  FileChunkIterator begin(FileChunkReader& r) { return r.begin(); }
  std::default_sentinel_t end(FileChunkReader& r) noexcept { return r.end(); }
};





void test_iterate()
{
    fmt::print("-> test_iterate reading lines from \"testfile.txt\"...\n");
    FileChunkReader fr("testfile.txt", '\n');
    auto i = fr.begin();
    static_assert(std::input_iterator<FileChunkIterator>);
    // not a  forward iterator static_assert(std::forward_iterator<FileChunkIterator>);
    //auto r = std::ranges::subrange(fr.begin(return ), fr.end());
    for (auto line : fr)
    {
      fmt::print("\tread line ({} chars): '{}\n'", line.length(), line);
    }
    fmt::print("...done.\n");
}




void test_range()
{
    fmt::print("-> test_range reading lines from \"testfile.txt\"...\n");
    FileChunkReader fr("testfile.txt", '\n');
    // strip newlines from ends, filter out short words:
    using transform_return_type = std::string;
    auto longwords = std::ranges::subrange(fr.begin(), fr.end()) 
      | std::ranges::views::transform(
        // we create a new std::string from the string view, so that each string is copied,
        // the string_view will become invalid on the next iteration of the iterator.
        // (though maybe this should be ok if when the range is iterated below it goes through each view
        // fully before starting the next)
        [](const std::string_view s) -> transform_return_type { 
          fmt::print("({}) -> ", s);
          transform_return_type r;
          if (s.ends_with("\n"))
            r = transform_return_type{s.begin(), s.end()-1};
          else
            r = transform_return_type{s};
          fmt::print("({})\n", r);
          return r;
        })
      | std::ranges::views::filter([](const auto& s){ return (s.length() > 3); });
    for (auto line : longwords)
    {
      fmt::print("\t>3 characters ({}): '{}'\n", line.length(), line);
    }
    fmt::print("...done.\n");
}





int main()
{
  test_iterate();
  test_range();
  return 0;
}

