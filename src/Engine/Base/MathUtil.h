#ifndef MATHUTIL_H_
#define MATHUTIL_H_


/**
 * Defines a math utility class.
 *
 * This is primarily used for optimized internal math operations.
 */
class MathUtil
{
    friend class Matrix;
    friend class Vector3;

public:
	static const float Pi;
	static const float Pi_2;
    static const float Pi_half;
	static const float Deg_Rad;
	static const float Rad_Deg;

    /**
     * Updates the given scalar towards the given target using a smoothing function.
     * The given response time determines the amount of smoothing (lag). A longer
     * response time yields a smoother result and more lag. To force the scalar to
     * follow the target closely, provide a response time that is very small relative
     * to the given elapsed time.
     *
     * @param x the scalar to update.
     * @param target target value.
     * @param elapsedTime elapsed time between calls.
     * @param responseTime response time (in the same units as elapsedTime).
     */
    static void smooth(float* x, float target, float elapsedTime, float responseTime);

    /**
     * Updates the given scalar towards the given target using a smoothing function.
     * The given rise and fall times determine the amount of smoothing (lag). Longer
     * rise and fall times yield a smoother result and more lag. To force the scalar to
     * follow the target closely, provide rise and fall times that are very small relative
     * to the given elapsed time.
     *
     * @param x the scalar to update.
     * @param target target value.
     * @param elapsedTime elapsed time between calls.
     * @param riseTime response time for rising slope (in the same units as elapsedTime).
     * @param fallTime response time for falling slope (in the same units as elapsedTime).
     */
    static void smooth(float* x, float target, float elapsedTime, float riseTime, float fallTime);

    template<typename T>
	static inline T (min)(T a, T b) { return a < b ? a : b; }

	template<typename T>
	static inline T (max)(T a, T b) { return a > b ? a : b; }

	static float sign(float val);

	static float clamp(float x, float lower, float upper);

	static float abs(float v);

	/// Returns the sine in radians from a lookup table.
	static float sin(float radians);

	/// Returns the cosine in radians from a lookup table.
	static float cos(float radians);

	/// Returns the sine in radians from a lookup table.
	static float sinDeg(float degrees);

	/// Returns the cosine in radians from a lookup table.
	static float cosDeg(float degrees);

	/// Returns atan2 in radians, faster but less accurate than Math.Atan2. Average error of 0.00231 radians (0.1323
	/// degrees), largest error of 0.00488 radians (0.2796 degrees).
	static float atan2(float y, float x);

	static float acos(float v);

	static float sqrt(float v);

	static float fmod(float a, float b);

	static bool isNan(float v);

	static float random();

	static float randomTriangular(float min, float max);

	static float randomTriangular(float min, float max, float mode);

	static float pow(float a, float b);
    
    static bool isPowerOfTwo(int x);

private:

    inline static void addMatrix(const float* m, float scalar, float* dst);

    inline static void addMatrix(const float* m1, const float* m2, float* dst);

    inline static void subtractMatrix(const float* m1, const float* m2, float* dst);

    inline static void multiplyMatrix(const float* m, float scalar, float* dst);

    inline static void multiplyMatrix(const float* m1, const float* m2, float* dst);

    inline static void negateMatrix(const float* m, float* dst);

    inline static void transposeMatrix(const float* m, float* dst);

    inline static void transformVector4(const float* m, float x, float y, float z, float w, float* dst);

    inline static void transformVector4(const float* m, const float* v, float* dst);

    inline static void crossVector3(const float* v1, const float* v2, float* dst);

    MathUtil();
};

struct Interpolation {
	virtual float apply(float a) = 0;

	virtual float interpolate(float start, float end, float a) {
		return start + (end - start) * apply(a);
	}

	virtual ~Interpolation() {};
};

struct PowInterpolation: public Interpolation {
	PowInterpolation(int power): power(power) {
	}

	float apply(float a) {
		if (a <= 0.5f) return MathUtil::pow(a * 2.0f, (float)power) / 2.0f;
		return MathUtil::pow((a - 1.0f) * 2.0f, (float)power) / (power % 2 == 0 ? -2.0f : 2.0f) + 1.0f;
	}

	int power;
};

struct PowOutInterpolation: public Interpolation {
	PowOutInterpolation(int power): power(power) {
	}

	float apply(float a) {
		return MathUtil::pow(a - 1, (float)power) * (power % 2 == 0 ? -1.0f : 1.0f) + 1.0f;
	}

	int power;
};

#define MATRIX_SIZE ( sizeof(float) * 16)

#ifdef USE_NEON
#include "MathUtilNeon.inl"
#else
#include "MathUtil.inl"
#endif

#endif
