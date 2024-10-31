
#include "Vector2.h"
#include "Vector3.h"
#include <cassert>
#include <corecrt_math.h>
#include "Base.h"


Vector2::Vector2()
    : x(0.0f), y(0.0f)
{
}

Vector2::Vector2(float x, float y)
    : x(x), y(y)
{
}

Vector2::Vector2(const float* array)
{
    set(array);
}

Vector2::Vector2(const Vector2& p1, const Vector2& p2)
{
    set(p1, p2);
}

Vector2::Vector2(const Vector2& copy)
{
    set(copy);
}


Vector2::Vector2(const Vector3& copy) {
    set(copy.x, copy.y);
}

Vector2::~Vector2()
{
}

const Vector2& Vector2::zero()
{
    static Vector2 value(0.0f, 0.0f);
    return value;
}

const Vector2& Vector2::one()
{
    static Vector2 value(1.0f, 1.0f);
    return value;
}

const Vector2& Vector2::unitX()
{
    static Vector2 value(1.0f, 0.0f);
    return value;
}

const Vector2& Vector2::unitY()
{
    static Vector2 value(0.0f, 1.0f);
    return value;
}

float crossProduct2Vector(const Vector2 &A, const Vector2 &B, const Vector2 &C, const Vector2 &D)
{
    return (D.y - C.y) * (B.x - A.x) - (D.x - C.x) * (B.y - A.y);
}

bool Vector2::isLineIntersect(const Vector2 &A, const Vector2 &B,
                              const Vector2 &C, const Vector2 &D,
                              float *S, float *T) {
    // FAIL: Line undefined
    if ( (A.x==B.x && A.y==B.y) || (C.x==D.x && C.y==D.y) ) {
        return false;
    }
    
    const float denom = crossProduct2Vector(A, B, C, D);
    
    if (denom == 0) {
        // Lines parallel or overlap
        return false;
    }
    
    if (S != nullptr) *S = crossProduct2Vector(C, D, C, A) / denom;
    if (T != nullptr) *T = crossProduct2Vector(A, B, C, A) / denom;
    
    return true;
}

bool Vector2::isZero() const
{
    return x == 0.0f && y == 0.0f;
}

bool Vector2::isOne() const
{
    return x == 1.0f && y == 1.0f;
}

float Vector2::angle(const Vector2& v1, const Vector2& v2)
{
    float dz = v1.x * v2.y - v1.y * v2.x;
    return atan2f(fabsf(dz) + MATH_FLOAT_SMALL, dot(v1, v2));
}

void Vector2::add(const Vector2& v)
{
    x += v.x;
    y += v.y;
}

void Vector2::add(const Vector2& v1, const Vector2& v2, Vector2* dst)
{
    assert(dst);

    dst->x = v1.x + v2.x;
    dst->y = v1.y + v2.y;
}

void Vector2::clamp(const Vector2& min, const Vector2& max)
{
    assert(!(min.x > max.x || min.y > max.y ));

    // Clamp the x value.
    if (x < min.x)
        x = min.x;
    if (x > max.x)
        x = max.x;

    // Clamp the y value.
    if (y < min.y)
        y = min.y;
    if (y > max.y)
        y = max.y;
}

void Vector2::clamp(const Vector2& v, const Vector2& min, const Vector2& max, Vector2* dst)
{
    M_ASSERT(dst);
    M_ASSERT(!(min.x > max.x || min.y > max.y ));

    // Clamp the x value.
    dst->x = v.x;
    if (dst->x < min.x)
        dst->x = min.x;
    if (dst->x > max.x)
        dst->x = max.x;

    // Clamp the y value.
    dst->y = v.y;
    if (dst->y < min.y)
        dst->y = min.y;
    if (dst->y > max.y)
        dst->y = max.y;
}

float Vector2::distance(const Vector2& v) const
{
    float dx = v.x - x;
    float dy = v.y - y;

    return sqrt(dx * dx + dy * dy);
}

float Vector2::distanceSquared(const Vector2& v) const
{
    float dx = v.x - x;
    float dy = v.y - y;
    return (dx * dx + dy * dy);
}

float Vector2::dot(const Vector2& v) const
{
    return (x * v.x + y * v.y);
}

float Vector2::dot(const Vector2& v1, const Vector2& v2)
{
    return (v1.x * v2.x + v1.y * v2.y);
}

float Vector2::length() const
{
    return sqrt(x * x + y * y);
}

float Vector2::lengthSquared() const
{
    return (x * x + y * y);
}

void Vector2::negate()
{
    x = -x;
    y = -y;
}

Vector2& Vector2::normalize()
{
    normalize(this);
    return *this;
}

void Vector2::normalize(Vector2* dst) const
{
    M_ASSERT(dst);

    if (dst != this)
    {
        dst->x = x;
        dst->y = y;
    }

    float n = x * x + y * y;
    // Already normalized.
    if (n == 1.0f)
        return;

    n = sqrt(n);
    // Too close to zero.
    if (n < MATH_TOLERANCE)
        return;

    n = 1.0f / n;
    dst->x *= n;
    dst->y *= n;
}

void Vector2::scale(float scalar)
{
    x *= scalar;
    y *= scalar;
}

void Vector2::scale(const Vector2& scale)
{
    x *= scale.x;
    y *= scale.y;
}

void Vector2::rotate(const Vector2& point, float angle)
{
    float sinAngle = sin(angle);
    float cosAngle = cos(angle);

    if (point.isZero())
    {
        float tempX = x * cosAngle - y * sinAngle;
        y = y * cosAngle + x * sinAngle;
        x = tempX;
    }
    else
    {
        float tempX = x - point.x;
        float tempY = y - point.y;

        x = tempX * cosAngle - tempY * sinAngle + point.x;
        y = tempY * cosAngle + tempX * sinAngle + point.y;
    }
}

Vector2 Vector2::lerp(const Vector2& other, float alpha)
{
    return *this * (1.f - alpha) + other * alpha;
}

void Vector2::set(float x, float y)
{
    this->x = x;
    this->y = y;
}

void Vector2::set(const float* array)
{
    M_ASSERT(array);

    x = array[0];
    y = array[1];
}

void Vector2::set(const Vector2& v)
{
    this->x = v.x;
    this->y = v.y;
}

void Vector2::set(const Vector2& p1, const Vector2& p2)
{
     x = p2.x - p1.x;
     y = p2.y - p1.y;
}

void Vector2::subtract(const Vector2& v)
{
    x -= v.x;
    y -= v.y;
}

void Vector2::subtract(const Vector2& v1, const Vector2& v2, Vector2* dst)
{
    M_ASSERT(dst);

    dst->x = v1.x - v2.x;
    dst->y = v1.y - v2.y;
}

void Vector2::smooth(const Vector2& target, float elapsedTime, float responseTime)
{
    if (elapsedTime > 0)
    {
        *this += (target - *this) * (elapsedTime / (elapsedTime + responseTime));
    }
}

void Vector2::reflect(const Vector2& normal)
{
    set(*this - 2.0f * this->dot(normal) * normal);
}