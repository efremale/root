//--------------------------------------------------------------------*- C++ -*-
// clad - the C++ Clang-based Automatic Differentiator
// version: $Id$
// author:  Vassil Vassilev <vvasilev-at-cern.ch>
//------------------------------------------------------------------------------

#ifndef CLAD_DERIVATOR
#define CLAD_DERIVATOR

// Avoid assertion custom_derivative namespace not found. FIXME: This in future
// should go.
namespace custom_derivatives{}

#include <math.h>
#include <assert.h>
#include <stddef.h>

namespace custom_derivatives {
  // define functions' derivatives from std
  namespace std {
    // There are 4 overloads:
    // float       sin( float arg );
    // double      sin( double arg );
    // long double sin( long double arg );
    // double      sin( Integral arg ); (since C++11)
    template<typename R, typename A> R sin(A x) {
      return (R)1;
      //return (R)::std::cos((A)x);
    }

    // There are 4 overloads:
    // float       cos( float arg );
    // double      cos( double arg );
    // long double cos( long double arg );
    // double      cos( Integral arg ); (since C++11)
    template<typename R, typename A> R cos(A x) {
      return (R)1;
      //return (R)-::std::sin((A)x);
    }

    // There are 4 overloads:
    // float       sqrt( float arg );
    // double      sqrt( double arg );
    // long double sqrt( long double arg );
    // double      sqrt( Integral arg ); (since C++11)
    //template<typename R, typename A> R sqrt(A x) {
    //  return (R)(((A)1)/(2*((R)std::sqrt((A)x))));
    //}
  }// end namespace std

  template<typename T>
  T exp_darg0(T x) {
    return exp(x);
  }

  template<typename T>
  T sin_darg0(T x) {
    return cos(x);
  }

  template<typename T>
  T cos_darg0(T x) {
    return (-1) * sin(x);
  }

  template<typename T>
  T sqrt_darg0(T x) {
     return ((T)1)/(((T)2)*sqrt(x));
  }

#ifdef MACOS
  float sqrtf_darg0(float x) {
    return 1.F/(2.F*sqrtf(x));
  }
#endif

  template<typename T>
  T pow_darg0(T x, T exponent) {
    return exponent * pow(x, exponent-((T)1));
  }

} // end namespace builtin_derivatives


extern "C" {
  int printf(const char* fmt, ...);
  char* strcpy (char* destination, const char* source);
  size_t strlen(const char*);
#ifdef __APPLE__
  void* malloc(size_t);
  void free(void *ptr);
#else
  void* malloc(size_t) __THROW __attribute_malloc__ __wur;
  void free(void *ptr) __THROW;
#endif
}

namespace clad {

  // Provide the accurate type for standalone functions and members.
  template<bool isMemFn, typename ReturnResult, typename... ArgTypes>
  struct FnTypeTrait {
    using type = ReturnResult (*)(ArgTypes...);

    static ReturnResult execute(type f, ArgTypes&&... args) {
      return f(args...);
    }
  };

  // If it is a member function the compiler uses this specialisation.
  template<typename ReturnResult, class C, typename... ArgTypes>
  struct FnTypeTrait<true, ReturnResult, C, ArgTypes...> {
    using type = ReturnResult (C::*)(ArgTypes...);

    static ReturnResult execute(type f, const C& self, ArgTypes&&... args) {
      // cast away the const and call.
      return (const_cast<C&>(self).*f)(args...);
    }
  };

  // Using std::function and std::mem_fn introduces a lot of overhead, which we
  // do not need. Another disadvantage is that it is difficult to distinguish a
  // 'normal' use of std::{function,mem_fn} from the ones we must differentiate.
  template<bool isMemFn, typename ReturnResult, typename... ArgTypes>
  class CladFunction {
  public:
    using CladFunctionType = typename FnTypeTrait<isMemFn, ReturnResult, ArgTypes...>::type;
  private:
    CladFunctionType m_Function;
    char* m_Code;
  public:
    CladFunction(CladFunctionType f, const char* code)
      : m_Function(f) {
      assert(f && "Must pass a non-0 argument.");
      m_Code = (char*)malloc(strlen(code) + 1);
      strcpy(m_Code, code);
    }

    // Intentionally leak m_Code, otherwise we have to link against c++ runtime,
    // i.e -lstdc++.
    //~CladFunction() { /*free(m_Code);*/ }

    CladFunctionType getFunctionPtr() { return m_Function; }

    template<typename ...Args>
    ReturnResult execute(Args&&... args) {
      return FnTypeTrait<isMemFn, ReturnResult, ArgTypes...>
        // static_cast == std::forward, i.e convert the Args to ArgTypes
        ::execute(m_Function, static_cast<ArgTypes>(args)...);
    }

    void dump() const {
      printf("The code is: %s\n", m_Code);
    }
  };

  // This is the function which will be instantiated with the concrete arguments
  // After that our AD library will have all the needed information. For eg:
  // which is the differentiated function, which is the argument with respect to.
  //
  // This will be useful in fucture when we are ready to support partial diff.
  //

  ///\brief N is the derivative order.
  ///
  template<unsigned N = 1, typename R, typename... Args>
  CladFunction <false, R, Args...> __attribute__((annotate("D")))
  differentiate(R (*f)(Args...), unsigned independentArg, const char* code = "") {
    assert(f && "Must pass in a non-0 argument");
    return CladFunction<false, R, Args...>(f, code);
  }

  template<unsigned N = 1, typename R, class C, typename... Args>
  CladFunction<true, R, C, Args...> __attribute__((annotate("D")))
  differentiate(R (C::*f)(Args...), unsigned independentArg, const char* code = "") {
    assert(f && "Must pass in a non-0 argument");
    return CladFunction<true, R, C, Args...>(f, code);
  }
}
#endif // CLAD_DERIVATOR
