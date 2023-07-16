

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
//  FileChunkReader which also provides new begin and end FileChunkIterators; each iterator contains its own file access handle FileHandle
//  (just a FILE* wrapper).  Each FileChunkIterator when advanced reads the next chunk into its own buffer, which can be accessed with
//  operator*.  
//
//  An alternative design would be instead for FileChunkReader to be the only reader of the file, reading new data on request from
//  any iterator, and providing data from its own buffer.  This would be a bit simpler and might be the more common use case, i.e. multiple
//  readers (multiple iterators) may not really be neccesary..
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





// Just wraps a FILE*, given a file descriptor. Closes the file pointer when destroyed.
// Maybe this could be replaced with fstream.
struct FileHandle
{
  FILE *fp = nullptr;

  explicit FileHandle(int fd) 
  {
    fp = fdopen(fd, "r");
    clearerr(fp);
    rewind(fp);
  }

  // take provided FILE*. Will be closed on destruction.
  explicit FileHandle(FILE* fp_) : fp(fp_) {}

  // we could maybe allow copying and either open a new FILE* from the same file using fdopen(fileno(fp)), or just copy the file pointer and let both FileHandle objects access it?
  FileHandle(const FileHandle& other) = delete;  
  FileHandle& operator=(const FileHandle& other) = delete;
    
  FileHandle(FileHandle&& old) 
  {
    fp = old.fp;
    old.fp = nullptr;
  }

  FileHandle& operator=(FileHandle&& old)
  {
    if(&old != this)
    {
      fp = old.fp;
      old.fp = nullptr;
    }
    return *this;
  }

  bool operator==(const FileHandle& other) const noexcept { return (other.fp == fp); }

  ~FileHandle() { if(fp) fclose(fp); }

  FILE* operator*() { return fp; }

  bool eof() const { return feof(fp); }
};






class FileChunkReader;



class FileChunkIteratorSentinel
{
};



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
    FileHandle file;
    int delimiter = 0;
    char *buf = nullptr; // allocated by getdelim(), but we must free in destructor
    size_t buflen = 0;
    difference_type counter = 0;

public:

    // note, reads first line, may throw
    FileChunkIterator(FileHandle file_, int delim_) : 
        file(std::move(file_)), delimiter(delim_)
    {
      read_line();
    }

    // note, reads first line, may throw
    FileChunkIterator(int fd_, int delim_) :
        file(fd_), delimiter(delim_)
    {
      read_line();
    }
    
    // not copyable
    FileChunkIterator(const FileChunkIterator& other) = delete;
    FileChunkIterator& operator=(const FileChunkIterator& other) = delete; 

    FileChunkIterator(FileChunkIterator&& old) noexcept :
        file(std::move(old.file)),
        delimiter(old.delimiter),
        buf(old.buf),
        buflen(old.buflen)
    {
      old.buf = nullptr;
      old.buflen = 0;
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
    // read next line into buffer (buf/buflen)
    // todo move to FileChunkReader
    void read_line()
    {
      // todo error if file at eof. (throw?)
      ssize_t r = getdelim(&buf, &buflen, delimiter, file.fp);
      if(r == -1) [[unlikely]]
      {
        // causes operator* to return a null string view:
        free(buf);
        buf = NULL;
        buflen = 0;

        if(file.eof())
        {
          //eof = true;
        }
        else
        {
          throw std::system_error(errno, std::system_category(), STRERROR(errno));
        }

      }
    }

    void advance()
    {
      // todo should this work move to operator*? (would also need to check counter vs. increment request)
      read_line();
      ++counter;
    }

public:

    const value_type operator*() const noexcept
    { 
      // Currently the first line is read by the constructor. this could instead be done here. But would that
      // violate 'const'? Or is it ok if a 'const FileChunkIterator' can't be incremented with ++, but still
      // does internal stuff like file IO and updating 'counter'?
      return std::string_view(buf, buflen);
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
      return (lhs.file == rhs.file) && (lhs.counter == rhs.counter);
    }

    bool eof() const noexcept
    {
      return file.eof();
    }

    // This is how end of the iteration is detected (e.g. 'if(i == std::end(reader))': The FileChunkIterator 'i' is the same as 'end()' (which returns a FileChunkIteratorSentinel) if it is at EOF.
    friend bool operator==(const FileChunkIterator& i, const FileChunkIteratorSentinel&) noexcept
    {
      return i.eof();
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

  ~FileChunkReader()
  {
    if(fd != -1)
    {
      int err = close(fd);
      assert(err != 0);
      //if(err != 0)
      //  throw std::system_error(errno, std::system_category(), STRERROR(errno));
    }
  }
    

  FileChunkIterator begin()
  {
    // create a new FileHandle from our file descriptor and set position to beginning of file. (This should not affect other FILE* pointers used in any other iterators.)
    return FileChunkIterator(FileHandle(fd), delimiter);
  }

  FileChunkIteratorSentinel end()
  {
    // todo should we also pass in fd so that FileChunkIterator::operator==() can check that too?
    return FileChunkIteratorSentinel{};
  }

};

// add overloads for std::begin() and std::end():
namespace std 
{
  FileChunkIterator begin(FileChunkReader& r) { return r.begin(); }
  FileChunkIteratorSentinel end(FileChunkReader& r) { return r.end(); }
};





void test_iterate()
{
    fmt::print("-> test_iterate reading lines from \"testfile.txt\"...\n");
    FileChunkReader fr("testfile.txt", '\n');
    auto i = fr.begin();
    static_assert(std::input_iterator<FileChunkIterator>);
    // not a  forward iterator static_assert(std::forward_iterator<FileChunkIterator>);
    //auto r = std::ranges::subrange(fr.begin(), fr.end());
    for (auto &line : fr)
    {
      fmt::print("\tread line: '{}\n'", line);
    }
    fmt::print("...done.\n");
}




// TODO why won't subrange() accept our iterators?
/*
void test_range()
{
    fmt::print("-> test_range reading lines from \"testfile.txt\"...\n");
    FileChunkReader fr("testfile.txt", '\n');
    // strip newlines from ends, filter out short words:
    auto longwords = std::ranges::subrange(fr.begin(), fr.end()) 
      | std::ranges::transform_view(
        [](const std::string_view s) -> std::string_view { 
          if (s.ends_with("\n"))
            return std::string_view(s, s.length()-1);
          else
            return s;
          // Note if string_view from iterator wasn't const we could use remove_suffix() instead.
        })
      | std::ranges::filter_view([](const auto s){ return (s.length() > 3); });
    for (auto &line : longwords)
    {
      fmt::print("\t>3 characters: '{}\n'", line);
    }
    fmt::print("...done.\n");
}
*/





int main()
{
  test_iterate();
  return 0;
}

