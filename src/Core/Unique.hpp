//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_UNIQUE_HPP
#  define OPEN_GLASSBOX_UNIQUE_HPP

#  include <memory>

// *****************************************************************************
// C++11 Implementation of the C++14 std::make_unique
// *****************************************************************************
#if !((defined(_MSC_VER) && (_MSC_VER >= 1800)) ||                                    \
      (defined(__clang__) && defined(__APPLE__) && (COMPILER_VERSION >= 60000)) ||    \
      (defined(__clang__) && (!defined(__APPLE__)) && (COMPILER_VERSION >= 30400)) && (__cplusplus > 201103L) || \
      (defined(__GNUC__) && (COMPILER_VERSION >= 40900) && (__cplusplus > 201103L)))

// These compilers do not support make_unique so redefine it
namespace std
{
  template<class T> struct _Unique_if
  {
    typedef unique_ptr<T> _Single_object;
  };

  template<class T> struct _Unique_if<T[]>
  {
    typedef unique_ptr<T[]> _Unknown_bound;
  };

  template<class T, size_t N> struct _Unique_if<T[N]>
  {
    typedef void _Known_bound;
  };

  template<class T, class... Args>
  typename _Unique_if<T>::_Single_object
  make_unique(Args&&... args)
  {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
  }

  template<class T>
  typename _Unique_if<T>::_Unknown_bound
  make_unique(size_t n)
  {
    typedef typename remove_extent<T>::type U;
    return unique_ptr<T>(new U[n]());
  }

  //! \brief Implement the C++14 std::make_unique for C++11
  template<class T, class... Args>
  typename _Unique_if<T>::_Known_bound
  make_unique(Args&&...) = delete;
}
#  endif

#endif
