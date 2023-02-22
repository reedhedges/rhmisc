#pragma once

/* Replacements for C assert() macros that write message and stack trace to stderr, and throw an exception (easier to catch in tests):
        rhm_assert(condition)         If (condition) is true, do nothing, otherwise print error message and throw an rhm::failed_assertion exception.
        rhm_precondition(condition)   Same as rhm_assert() but note that it was a precondition in the error message.
        rhm_postcondition(condition)  Same as rhm_assert() but note that it was a postcondition in the error message.
        
   If std::stacktrace is available (C++23), then a stack trace is printed and stored in the exception.
   If std::source_location is available (C++20) then it will be used instead of the old __FILE__, __LINE__ and __PRETTY_FUNCTION__ macros.
   (Note, rhm_assert etc. are still macros so we can get a string representation of the checked expression.)
   Requires fmt library.

  * TODO add easy integration point with a logging facility.
*/

#include <exception>
#include <iostream>
#include <cstdio>
#include <string>
#include <string_view>
#include <optional>

#include "fmt/format.h" // todo replace with std::format once gcc and clang that support it are more widely available (gcc 13, clang 14, msvc 19.29).

#if __has_include("stacktrace")
#include <stacktrace>
#endif

namespace rhm
{

  enum assertion_type {
      assertion,
      precondition,
      postcondition
  };

  class failed_assertion : public std::runtime_error
  {
    public:
    #ifdef __cpp_lib_stacktrace
      failed_assertion(std::string_view what, std::string_view e, std::string_view file, unsigned int l, std::string_view func, assertion_type t, const std::stacktrace& strace) noexcept : 
        std::runtime_error(std::string(what)),
        expression(e),
        file_name(file),
        line(l),
        function_name(func),
        assert_type(t),
        stack_trace(strace)
      {}
      #endif
      failed_assertion(std::string_view what, std::string_view e, std::string_view file, unsigned int l, std::string_view func, assertion_type t) noexcept : 
        std::runtime_error(std::string(what)),
        expression(e),
        file_name(file),
        line(l),
        function_name(func),
        assert_type(t)
      {}

      std::string expression;
      std::string file_name;
      unsigned int line;
      std::string function_name;
      rhm::assertion_type assert_type;
      #ifdef __cpp_lib_stacktrace
        std::optional<std::stacktrace> stack_trace;
      #endif
  };


  namespace _private
  {
            
    inline std::string_view assertion_type_to_string(assertion_type t) {
        switch(t) {
            case assertion: return "Assertion";
            case precondition: return "Precondition";
            case postcondition: return "Postcondition";
        }
        #ifdef __cpp_lib_unreachable
            std::unreachable();
        #else
            abort();
            return "?";
        #endif
    }

    [[noreturn]] inline void assert_fail(const char *expression, const char *file, unsigned int line, const char *function, rhm::assertion_type assert_type) // todo stack trace
    {
      std::string msg = fmt::format("{}:{} : {}: {} ({}) failed. [{} +{}]", file, line, function, assertion_type_to_string(assert_type), expression, file, line);
      // todo add color code characters if we might be running in a terminal (check environment variables or other hints.)
      std::string sep(78, '-');
      std::cerr << "\n" << sep << "\n" << msg << "\n" << sep << '\n'; 
      fflush(stderr);
      #ifdef __cpp_lib_stacktrace
        std::stacktrace strace(std::stacktrace::current());
        std::cerr << "Stack trace:\n" << strace << "\n" << sep << "\n\n";
        fflush(stderr);
        throw failed_assertion(msg, expression, file, line, function, assert_type, strace);
      #else
        throw failed_assertion(msg, expression, file, line, function, assert_type);
      #endif
    }
  }


}

#if __has_include("source_location")
#include <source_location>
#endif

#ifdef __cpp_lib_source_location
// todo check feature flags and fall back on __FILE__, __LINE__, and __PRETTY_FUNCTION__ if we don't have source_location.
    #define _rhm_file (std::source_location::current().file_name())
    #define _rhm_line (std::source_location::current().line())
    #define _rhm_func (std::source_location::current().function_name())
#else
    #define _rhm_file __FILE__
    #define _rhm_line __LINE__
    #define _rhm_func __PRETTY_FUNCTION__
#endif

#define rhm_assert(expr) ( static_cast<bool>(expr) ? void(0) : rhm::_private::assert_fail(#expr, _rhm_file, _rhm_line, _rhm_func, rhm::assertion) )
#define rhm_assert_precond(expr) ( static_cast<bool>(expr) ? void(0) : rhm::_private::assert_fail(#expr, _rhm_file, _rhm_line, _rhm_func, rhm::precondition) )
#define rhm_assert_postcond(expr) ( static_cast<bool>(expr) ? void(0) : rhm::_private::assert_fail(#expr, _rhm_file, _rhm_line, _rhm_func, rhm::postcondition) )

