
#ifndef Base_h
#define Base_h

#include <cctype>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <string>

#ifdef _WINDOWS
#include <assert.h>
#endif // _WINDOWS

#if defined(__clang__) || defined(__GNUC__)

#define CPP_STANDARD __cplusplus

#elif defined(_MSC_VER)

#define CPP_STANDARD _MSVC_LANG

#endif

#if CPP_STANDARD >= 199711L
#define HAS_CPP_03 1
#endif
#if CPP_STANDARD >= 201103L
#define HAS_CPP_11 1
#endif
#if CPP_STANDARD >= 201402L
#define HAS_CPP_14 1
#endif
#if CPP_STANDARD >= 201703L
#define HAS_CPP_17 1
#endif

using std::va_list;
using std::memcpy;
using std::fabs;
using std::sqrt;
using std::cos;
using std::sin;
using std::tan;
using std::isspace;
using std::isdigit;
using std::toupper;
using std::tolower;
using std::size_t;
using std::min;
using std::max;
using std::modf;
using std::atoi;

// Assert macros.
#ifdef _DEBUG
#define M_ASSERT(expression) assert(expression)
#else
#define M_ASSERT(expression)
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(x) { if (x) free(x); (x) = NULL; }	//定义安全释放函数
#endif
#ifndef SAFE_DELETE
#define SAFE_DELETE(x) { if (x) delete (x); (x) = NULL; }	//定义安全释放函数
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) { if (x) delete [] (x); (x) = NULL; }	//定义安全释放函数
#endif
#ifndef SAFE_RETAIN
#define SAFE_RETAIN(x) { if (x) (x)->retain(); }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) { if (x) (x)->release(); x = nullptr; }
#endif
#ifndef SAFE_UNREF
#define SAFE_UNREF(x) { if (x && x->unref()) { x->reset();  delete x; } x = nullptr; }
#endif
#ifndef SAFE_REF
#define SAFE_REF(x) { if (x) { x->ref(); } }
#endif

#if defined(__GNUC__) && ((__GNUC__ >= 5) || ((__GNUG__ == 4) && (__GNUC_MINOR__ >= 4))) \
    || (defined(__clang__) && (__clang_major__ >= 3)) || (_MSC_VER >= 1800)
#define BK_DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName &) = delete; \
    TypeName &operator =(const TypeName &) = delete;
#else
#define BK_DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName &); \
    TypeName &operator =(const TypeName &);
#endif

// Math
#define MATH_DEG_TO_RAD(x)          ((x) * 0.0174532925f)
#define MATH_RAD_TO_DEG(x)          ((x)* 57.29577951f)
#define MATH_RANDOM_MINUS1_1()      ((2.0f*((float)rand()/RAND_MAX))-1.0f)      // Returns a random float between -1 and 1.
#define MATH_RANDOM_0_1()           ((float)rand()/RAND_MAX)                    // Returns a random float between 0 and 1.
#define MATH_FLOAT_SMALL            1.0e-37f
#define MATH_TOLERANCE              2e-37f
#define MATH_E                      2.71828182845904523536f
#define MATH_LOG10E                 0.4342944819032518f
#define MATH_LOG2E                  1.442695040888963387f
#define MATH_PI                     3.14159265358979323846f
#define MATH_PIOVER2                1.57079632679489661923f
#define MATH_PIOVER4                0.785398163397448309616f
#define MATH_PIX2                   6.28318530717958647693f
#define MATH_EPSILON                0.000001f
#define MATH_CLAMP(x, lo, hi)       ((x < lo) ? lo : ((x > hi) ? hi : x))
#ifndef M_1_PI
#define M_1_PI                      0.31830988618379067154
#endif

inline float clampf(float value, float min_inclusive, float max_inclusive)
{
	if (min_inclusive > max_inclusive) {
		std::swap(min_inclusive, max_inclusive);
	}
	return value < min_inclusive ? min_inclusive : value < max_inclusive ? value : max_inclusive;
}

#endif /* Base_h */
