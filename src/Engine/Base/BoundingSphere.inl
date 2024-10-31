#ifndef BoundingSphere_inl
#define BoundingSphere_inl

#include "BoundingSphere.h"

inline BoundingSphere& BoundingSphere::operator*=(const Matrix& matrix)
{
    transform(matrix);
    return *this;
}

inline const BoundingSphere operator*(const Matrix& matrix, const BoundingSphere& sphere)
{
    BoundingSphere s(sphere);
    s.transform(matrix);
    return s;
}

#endif // BoundingSphere_inl
