#ifndef Plane_inl
#define Plane_inl

#include "Plane.h"



inline Plane& Plane::operator*=(const Matrix& matrix)
{
    transform(matrix);
    return *this;
}

inline const Plane operator*(const Matrix& matrix, const Plane& plane)
{
    Plane p(plane);
    p.transform(matrix);
    return p;
}

#endif // Plane_inl
