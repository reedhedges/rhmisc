#pragma once

#include <string>
#include <string_view>
#include <limits>
#include <algorithm>

namespace rhm {

namespace _private {
#ifdef _LIBCPP_VERSION // will be defined in some library header file included by <string> or one of the others
constexpr size_t sso_len = 22;
#else
constexpr size_t sso_len = 15;
#endif
}

// TODO we can get length of a char* string with std::char_traits<char>::length()! (And wide
// strings with std::char_traits<wchart_t>::length()!)  This might remove the need for 
// the string_literal<N> class.

// Constexpr wrapper class for string literals, should be constructed with a string literal, and provides char* and length values.
// The constructor copies data to a char buffer (should be done at compile time in constexpr constructor), and length is also automatically deduced at compile time from string literal used to initialize.
// This is meant to be used in a constexpr context (in which case data copied into char buffer and all other operations are done at compile time),
// and passed to other functions in the constexpr context, e.g. as template value parameters.
// Can be implicitly cast to a std::string_view.
// For example:
//      template<string_literal s> 
//      size_t count(char c) { 
//          size_t sum = 0; 
//          for(int i = 0; i < s.length(); ++i) 
//              if(s.data()[i] == c) 
//                  ++sum; 
//          return sum; 
//      }
//     ...
//      char c = 'o';
//      size_t n = count<"hello world">(c);
//      
// Adapted from blog post by Kevin Hartmann <https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/>
template<size_t N> // N is size of string literal including null terminator, and can be deduced from 'str' given in constructor (after C++17)
struct string_literal {
    consteval string_literal(const char (&str)[N]) {
        // This constructor is constexpr and so with str as a reference to string literal with known size and 
        // constexpr std::copy_n() as the only statement in the body, it should be evaluated at compile time to initialize 'value'.
        // (Any non constant-evaluated statements in here would cause the template deduction to fail.)
        std::copy_n(str, N, value);
    }
    constexpr const char *data() const { return value; }
    constexpr const char *c_str() const { return value; }
    constexpr size_t length() const { return N; }
    constexpr size_t size() const { return N; }
    constexpr operator std::string_view() const { return std::string_view(data(), length()); }

//private: // must not have any private members to remain a structural class template
    char value[N];
};


// Produce a std::string from a string literal (e.g. "my string", passed as a string_literal,
// defined above, template parameter) followed by a non-string value, converted to a string using 
// std::to_string.
// Chooses (at compile time) the fastest method to do so based on either the given value n, or the
// length of the resulting std::string (computed at compile time): If small enough for "small 
// string optimization" (SSO), then simply construct the std::string and append the value. Otherwise,
// reserve memory for the required size (with std::string::reserve())  first, then append the
// string and value. 
// If we are using libc++ (Clang/LLVM), then the maximum size used for small string optimization
// is 22. Otherwise (GNU libstdc++, MSVC, etc.), it is 15.
// If the n parameter is 0, then the maximum possible size in characters of val is computer (std::numeric_limits<T>::digits).
// Otherwise, it gives a typical or likely size of the value converted to a string, this, plus the length of cstr,
// is used as the approximate size of the std::string created and returned (Note, if too small, then std::string will have to do additional allocation,
// but if too large, then the string will not be assumed to qualify for SSO.) 
template<string_literal cstr, size_t n = 0, typename T> // T will be deduced from the val argument. 
constexpr std::string append_to_string_literal(T val)
{
    constexpr size_t len = cstr.length() + ( (n == 0) ? std::numeric_limits<T>::digits : n );
    if(len <= rhm::_private::sso_len)
    {
        std::string s(cstr.data(), cstr.length());
        s.append(std::to_string(val));
        return s;
    }
    else
    {
        std::string s;
        s.reserve(len);
        s += cstr.data();
        s += std::to_string(val);
        return s;
    }
}




} // end namespace rhm


 
/*
// An example function using string_literal parameter:
template<rhm::string_literal lit>
void print_literal() {
    // The size of the string is available as a constant expression.
    constexpr auto size = lit.length();

    // and so is the string's content.
    constexpr auto contents = lit.data();

    fmt::print("String={}, size={}.\n", contents, size);
    
}
*/
