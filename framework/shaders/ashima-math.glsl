// Copyright 2012 Ashima Arts <http://ashimaarts.com/>
// License: BSD-3-Clause <http://www.opensource.org/licenses/BSD-3-Clause>
// math functions defined in later GLSL specs

#if 1
#define SINH(T) \
  T sinh(T x) { \
    T e = exp(x); \
    return 0.5 * (e - 1/e); \
  }

SINH(float)
SINH(vec2)
SINH(vec3)
SINH(vec4)
#undef SINH
#endif

#if 1
#define COSH(T) \
  T cosh(T x) { \
    T e = exp(x); \
    return 0.5 * (e + 1.0/e); \
    }

COSH(float)
COSH(vec2)
COSH(vec3)
COSH(vec4)
#undef COSH
#endif

#if 1
#define TANH(T) \
  T tanh(T x) { \
    T e = exp(2.0*x) ;\
    return ( e-1.0 )/ (e+1.0) ; \
    }

TANH(float)
TANH(vec2)
TANH(vec3)
TANH(vec4)
#undef TANH
#endif

#if 1
#define ASINH(T) \
  T asinh(T x) { \
    return log(x + sqrt(x*x+1.0)); \
    }

ASINH(float)
ASINH(vec2)
ASINH(vec3)
ASINH(vec4)
#undef ASINH
#endif

#if 1
#define ACOSH(T) \
  T acosh(T x) { \
    return log(x + sqrt(x*x-1.0)); \
    }

ACOSH(float)
ACOSH(vec2)
ACOSH(vec3)
ACOSH(vec4)
#undef ACOSH
#endif

#if 1
#define ATANH(T) \
  T atanh(T x) { \
    return 0.5 * log( (x+1) / (x-1)); \
    }

ATANH(float)
ATANH(vec2)
ATANH(vec3)
ATANH(vec4)
#undef ATANH
#endif

#if 1
// Need to find a neater way of doing this without going via ints.
#define TRUNC(T) \
  T trunc(T x) {\
    return sign(x)*floor(abs(x)); \
    }

TRUNC(float)
TRUNC(vec2)
TRUNC(vec3)
TRUNC(vec4)
#undef TRUNC
#endif

#if 1
#define ROUND(T) \
  T round(T x) {\
    return floor(x + 0.5); \
  }

ROUND(float)
ROUND(vec2)
ROUND(vec3)
ROUND(vec4)
#undef ROUND
#endif

#if 1
#define ROUNDEVEN(T) \
  T roundEven(T x) { \
    T y = floor(x + 0.5); \
    return ( y - x == T(0.5) ) ? floor(0.5*y)*2.0 : y ; \
    }

ROUNDEVEN(float)
ROUNDEVEN(vec2)
ROUNDEVEN(vec3)
ROUNDEVEN(vec4)
#undef ROUNDEVEN
#endif

#if 1
#define FMA(T) \
  T fma(T a, T b, T c) { \
    return  a*b + c; \
    }

FMA(float)
FMA(vec2)
FMA(vec3)
FMA(vec4)
#undef FMA
#endif

#if 1
#define MODF(T) \
  T modf(T x, out T i) {\
    i = floor(x); \
    return x - i; \
    }

MODF(float)
MODF(vec2)
MODF(vec3)
MODF(vec4)
#undef MODF
#endif


