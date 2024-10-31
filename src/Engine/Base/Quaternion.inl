#ifndef Quaternion_inl
#define Quaternion_inl

#include "Quaternion.h"


inline const Quaternion Quaternion::operator*(const Quaternion& q) const
{
    Quaternion result(*this);
    result.multiply(q);
    return result;
}

inline Quaternion& Quaternion::operator*=(const Quaternion& q)
{
    multiply(q);
    return *this;
}

inline bool Quaternion::operator==(const Quaternion& q) const
{
	return x == q.x && y == q.y && z == q.z && w== q.w;
}

inline bool Quaternion::operator!=(const Quaternion& q) const
{
	return x != q.x || y != q.y || z != q.z || w == q.w;
}

#endif // Quaternion_inl
