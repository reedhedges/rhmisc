
#pragma once

#ifdef __STDC_VERSION_STDCKDINT_H__
//#warning Using stdckdint.h
#include <stdckdint.h>
#define _c_lib_stdckdint 1
#else
//#warning not using sdckdint.h, using compiler builtins instead.
#endif

// #if defined(__GNUC__) || defined(__clang__) // GCC or Clang
//   // Check for clang >= 3.8 or GCC >= 5
//   #if ( defined(__clang__) && ( (__clang_major__ > 3) || (__clang_major__ == 3 && __clang_minor__ >= 8) ) || ( !defined(__clang__) && (__GNUC__ >= 5) ) )
//     #define _have_builtin_add_overflow 1 // todo check for gcc5+ or clang3.8+
//   #endif
// #else
//   #error this compiler is not supported
// #endif

template <typename TR, typename T1, typename T2>
inline constexpr TR checked_add(T1 a, T2 b) noexcept
{
    TR result;
    #if defined(_c_lib_stdckdint)
        bool overflow = ckd_add(&result, a, b);
    #else
        bool overflow = __builtin_add_overflow(a, b, &result);
    #endif

    // todo compile time option to choose to either abort() or throw exception. (and need to remove noexcept)
    if(overflow) [[unlikely]]
        abort();
        //throw std::overflow_error("addition overflow"); 

    return result;
} 

template <typename TR, typename T1, typename T2>
inline constexpr TR checked_sub(T1 a, T2 b) noexcept
{
    TR result;
    #if defined(_c_lib_stdckdint)
        bool overflow = ckd_sub(&result, a, b);
    #else
        bool overflow = __builtin_sub_overflow(a, b, &result);
    #endif

    // todo compile time option to choose to either abort() or throw exception. (and need to remove noexcept)
    if(overflow) [[unlikely]]
        abort();
        //throw std::overflow_error("addition overflow"); 

    return result;
} 

template <typename TR, typename T1, typename T2>
inline constexpr TR checked_mul(T1 a, T2 b) noexcept
{
    TR result;
    #if defined(_c_lib_stdckdint)
        bool overflow = ckd_mul(&result, a, b);
    #else
        bool overflow = __builtin_mul_overflow(a, b, &result);
    #endif

    // todo compile time option to choose to either abort() or throw exception. (and need to remove noexcept)
    if(overflow) [[unlikely]]
        abort();
        //throw std::overflow_error("addition overflow"); 

    return result;
} 



/*
template <typename TR, typename T1, typename T2>
inline constexpr TR unchecked_add(T1 a, T2 b) noexcept
{
    return a+b;
}


#define CHECK
#include "fmt/format.h"
int main(int argc, char **argv)
{
   #ifdef CHECK
     int x = checked_add<int>(23, argc);
   #else
     int x = unchecked_add<int>(23, argc);
   #endif
   unsigned int a = 23;
   unsigned int b = argc;
   #ifdef CHECK
     int x2 = checked_add<int>(a, b);
   #else
     int x2 = unchecked_add<int>(a, b);
   #endif
   long long a2 = 5000000;
   long long b2 = argc;
   long r = checked_add<long>(a2, b2);
   long r2 = unchecked_add<long>(a2, b2);
   fmt::print("{}, {}, {}, {}\n", x,  x2,  r, r2);
   return 0;
}
*/

