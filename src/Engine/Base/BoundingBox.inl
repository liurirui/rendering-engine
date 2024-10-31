#ifndef BoundingBox_inl
#define BoundingBox_inl

#include "BoundingBox.h"



inline BoundingBox& BoundingBox::operator*=(const Matrix& matrix)
{
    transform(matrix);
    return *this;
}

inline const BoundingBox operator*(const Matrix& matrix, const BoundingBox& box)
{
    BoundingBox b(box);
    b.transform(matrix);
    return b;
}

#endif // BoundingBox_inl
