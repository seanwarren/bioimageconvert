/*****************************************************************************
 Base typing and type conversion definitions

 DEFINITION

 Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
 Copyright (c) 2006 Vision Research Lab, UCSB <http://vision.ece.ucsb.edu>

 History:
   04/19/2006 16:20 - First creation

 Ver : 1
*****************************************************************************/

#ifndef BIM_XTYPES
#define BIM_XTYPES
//#pragma message(">>>>>  xtypes: included types and type conversion utils")

#include <cmath>
#include <vector>
#include <limits>

namespace bim {

//------------------------------------------------------------------------------
// first define type macros, you can define them as needed, OS specific
//------------------------------------------------------------------------------
/*
Datatype  LP64  ILP64 LLP64 ILP32 LP32
char       8     8     8      8      8
short     16    16    16     16     16
_int32          32
int       32    64    32     32     16
long      64    64    32     32     32
long long       64
pointer   64    64    64     32     32
*/

#ifndef _BIM_TYPEDEFS_
#define _BIM_TYPEDEFS_

// system types
typedef	unsigned char uchar;
typedef	unsigned int uint;

// sized types
typedef	signed char int8;
typedef	unsigned char uint8;
typedef	short int16;
typedef	unsigned short uint16;

#if defined WIN32 || defined WIN64 || defined _WIN32 || defined _WIN64 || defined _MSVC
typedef __int32 int32;
typedef __int64 int64;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;
#else
typedef int int32;
typedef long long int64;
typedef unsigned int uint32;
typedef unsigned long long uint64;
#endif

typedef	float float32;
typedef	double float64;
typedef	long double float80;

#endif // _BIM_TYPEDEFS_


#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#if (defined WIN32 || defined WIN64 || defined _WIN32 || defined _WIN64 || defined _MSVC) && !defined(__MINGW32__)
#define BIM_WIN
#endif

//------------------------------------------------------------------------------
// BIG_ENDIAN is for SPARC, Motorola, IBM and LITTLE_ENDIAN for intel type
//------------------------------------------------------------------------------
const static int bimOne = 1;
const static int bigendian = (*(const char *)&bimOne == 0);
const static double Pi = 3.14159265358979323846264338327950288419716939937510;

#if defined(_MSC_VER)
#define BIM_RESTRICT __restrict
#define BIM_ALIGN(a) __declspec(align(a))
#elif defined(__GNUG__)
#define BIM_RESTRICT __restrict__
#define BIM_ALIGN(a) __attribute__((__aligned__(a)))
#else
#define BIM_RESTRICT
#define BIM_ALIGN(a)
#endif
#define BIM_ALIGN_INT8   BIM_ALIGN(8)
#define BIM_ALIGN_INT16  BIM_ALIGN(16)
#define BIM_ALIGN_INT32  BIM_ALIGN(32)
#define BIM_ALIGN_INT64  BIM_ALIGN(64)
#define BIM_ALIGN_FLOAT  BIM_ALIGN(32)
#define BIM_ALIGN_DOUBLE BIM_ALIGN(64)

#define BIM_OMP_FOR1 4000
#define BIM_OMP_FOR2 60
#define BIM_OMP_SCHEDULE schedule(runtime)

template<typename T>
inline bool isnan(T value) { return value != value; }

//------------------------------------------------------------------------------
// SWAP types for big/small endian conversions
//------------------------------------------------------------------------------

inline void swapShort(uint16* wp) {
  register uchar* cp = (uchar*) wp;
  uchar t;
  t = cp[1]; cp[1] = cp[0]; cp[0] = t;
}

inline void swapLong(uint32* lp) {
  register uchar* cp = (uchar*) lp;
  uchar t;
  t = cp[3]; cp[3] = cp[0]; cp[0] = t;
  t = cp[2]; cp[2] = cp[1]; cp[1] = t;
}

void swapArrayOfShort(uint16* wp, register uint64 n);
void swapArrayOfLong(register uint32* lp, register uint64 n);

inline void swapFloat(float32* lp) {
  register uchar* cp = (uchar*) lp;
  uchar t;
  t = cp[3]; cp[3] = cp[0]; cp[0] = t;
  t = cp[2]; cp[2] = cp[1]; cp[1] = t;
}

void swapDouble(float64 *dp);
void swapArrayOfFloat(float32* dp, register uint64 n);
void swapArrayOfDouble(float64* dp, register uint64 n);

//------------------------------------------------------------------------------
// min/max
//------------------------------------------------------------------------------

template<typename T>
inline const T& min(const T& a, const T& b) {
  return (a < b) ? a : b;
}

template<typename T>
inline const T& max(const T& a, const T& b) {
  return (a > b) ? a : b;
}

//------------------------------------------------------------------------------
// min/max for arrays
//------------------------------------------------------------------------------

template<typename T>
const T max(const T *a, unsigned int size) {
  T val = a[0];
  for (unsigned int i=0; i<size; ++i)
    if (val < a[i]) val = a[i];
  return val;
}

template<typename T>
const T max(const std::vector<T> &a) {
  T val = a[0];
  for (unsigned int i=0; i<a.size(); ++i)
    if (val < a[i]) val = a[i];
  return val;
}

template<typename T>
unsigned int maxix(const T *a, unsigned int size) {
  unsigned int i,ix;
  T val = a[0];
  ix = 0;
  for (i=0; i<size; ++i)
    if (val < a[i]) {
      val = a[i];
      ix = i;
    }
  return ix;
}

template<typename T>
const T min(const T *a, unsigned int size) {
  T val = a[0];
  for (unsigned int i=0; i<size; ++i)
    if (val > a[i]) val = a[i];
  return val;
}

template<typename T>
const T min(const std::vector<T> &a) {
  T val = a[0];
  for (unsigned int i=0; i<a.size(); ++i)
    if (val > a[i]) val = a[i];
  return val;
}

template<typename T>
unsigned int minix(const T *a, unsigned int size) {
  unsigned int i,ix;
  T val = a[0];
  ix = 0;
  for (i=0; i<size; ++i)
    if (val > a[i]) {
      val = a[i];
      ix = i;
    }
  return ix;
}

//------------------------------------------------------------------------------
// utils
//------------------------------------------------------------------------------

template <typename T>
inline T highest() {
    return std::numeric_limits<T>::max();
}

template <typename T>
inline T lowest() {
    return std::numeric_limits<T>::is_integer ? std::numeric_limits<T>::min() : -std::numeric_limits<T>::max();
    //return std::numeric_limits<T>::lowest(); // c++11 version
}

template <typename T>
inline T trim(T v, T min_v=std::numeric_limits<T>::is_integer?std::numeric_limits<T>::min():-std::numeric_limits<T>::max(), T max_v=std::numeric_limits<T>::max()) {
  if (v < min_v) return min_v;
  if (v > max_v) return max_v;
  return v;
}

template <typename To, typename Ti>
inline To trim( Ti val, To min=std::numeric_limits<To>::is_integer?std::numeric_limits<To>::min():-std::numeric_limits<To>::max(), To max=std::numeric_limits<To>::max() ) {
  if (val < min) return min;
  if (val > max) return max;
  return (To) val;
}

//------------------------------------------------------------------------------
// little math
//------------------------------------------------------------------------------

template <typename T>
inline T round( double x ) {
  return (T) floor(x + 0.5);
}

template <typename T>
inline T round( float x ) {
  return (T) floor(x + 0.5f);
}

// round number n to d decimal points
template <typename T>
inline T round(double x, int d) {
  return (T) floor(x * pow(10., d) + .5) / pow(10., d);
}

template <typename T>
T power(T base, int index) {

  if (index < 0) {
    return 1.0/pow( base, -index );
  }
  else
  return pow( base, index );
}

template <typename T>
inline T log2(T n) {
  return (T) (log((double)n)/log(2.0));
}

/*
template <typename T>
inline T ln(T n) {
  return (T) (log((double)n)/log(E));
}
*/

/*
float invSqrt (float x) {
  float xhalf = 0.5f*x;
  int i = *(int*)&x;
  i = 0x5f3759df - (i >> 1);
  x = *(float*)&i;
  x = x*(1.5f - xhalf*x*x);
  return x;
}
*/

} // namespace bim

#endif // BIM_XTYPES
