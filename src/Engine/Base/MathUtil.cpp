#include "Base.h"
#include "MathUtil.h"
#include <math.h>
#include <stdlib.h>



const float MathUtil::Pi = 3.1415926535897932385f;
const float MathUtil::Pi_2 = 3.1415926535897932385f * 2;
const float MathUtil::Pi_half = 3.1415926535897932385f / 2.0f;
const float MathUtil::Deg_Rad = (3.1415926535897932385f / 180.0f);
const float MathUtil::Rad_Deg = (180.0f / 3.1415926535897932385f);


void MathUtil::smooth(float* x, float target, float elapsedTime, float responseTime)
{
    M_ASSERT(x);

    if (elapsedTime > 0)
    {
        *x += (target - *x) * elapsedTime / (elapsedTime + responseTime);
    }
}

void MathUtil::smooth(float* x, float target, float elapsedTime, float riseTime, float fallTime)
{
    M_ASSERT(x);
    
    if (elapsedTime > 0)
    {
        float delta = target - *x;
        *x += delta * elapsedTime / (elapsedTime + (delta > 0 ? riseTime : fallTime));
    }
}

float MathUtil::abs(float v) {
	return ((v) < 0 ? -(v) : (v));
}

float MathUtil::sign(float v) {
	return ((v) < 0 ? -1.0f : (v) > 0 ? 1.0f : 0.0f);
}

float MathUtil::clamp(float x, float min, float max) {
	return ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)));
}

float MathUtil::fmod(float a, float b) {
	return (float)::fmod(a, b);
}

/// Returns atan2 in radians, faster but less accurate than Math.Atan2. Average error of 0.00231 radians (0.1323
/// degrees), largest error of 0.00488 radians (0.2796 degrees).
float MathUtil::atan2(float y, float x) {
	return (float)::atan2(y, x);
}

/// Returns the cosine in radians from a lookup table.
float MathUtil::cos(float radians) {
	return (float)::cos(radians);
}

/// Returns the sine in radians from a lookup table.
float MathUtil::sin(float radians) {
	return (float)::sin(radians);
}

float MathUtil::sqrt(float v) {
	return (float)::sqrt(v);
}

float MathUtil::acos(float v) {
	return (float)::acos(v);
}

/// Returns the sine in radians from a lookup table.
float MathUtil::sinDeg(float degrees) {
	return (float)::sin(degrees * MathUtil::Deg_Rad);
}

/// Returns the cosine in radians from a lookup table.
float MathUtil::cosDeg(float degrees) {
	return (float)::cos(degrees * MathUtil::Deg_Rad);
}

/* Need to pass 0 as an argument, so VC++ doesn't error with C2124 */
static bool __isNan(float value, float zero) {
	float _nan = (float) 0.0 / zero;
	return 0 == memcmp((void *) &value, (void *) &_nan, sizeof(value));
}

bool MathUtil::isNan(float v) {
	return __isNan(v, 0);
}

float MathUtil::random() {
	return ::rand() / (float)RAND_MAX;
}

float MathUtil::randomTriangular(float min, float max) {
	return randomTriangular(min, max, (min + max) * 0.5f);
}

float MathUtil::randomTriangular(float min, float max, float mode) {
	float u = random();
	float d = max - min;
	if (u <= (mode - min) / d) return min + sqrt(u * d * (mode - min));
	return max - sqrt((1 - u) * d * (max - mode));
}

float MathUtil::pow(float a, float b) {
	return (float)::pow(a, b);
}

bool MathUtil::isPowerOfTwo(int x) {
    bool result = x > 0 && (x & (x - 1)) == 0;
    return result;
}

