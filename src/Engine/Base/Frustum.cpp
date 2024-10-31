#include "Base.h"
#include "Frustum.h"
#include "BoundingSphere.h"
#include "BoundingBox.h"


Frustum::Frustum()
{
    set(Matrix::identity());
}

Frustum::Frustum(const Matrix& matrix)
{
    set(matrix);
}

Frustum::Frustum(const Frustum& frustum)
{
    set(frustum);
}

Frustum::~Frustum()
{
}

const Plane& Frustum::getNear() const
{
    return _near;
}

const Plane& Frustum::getFar() const
{
    return _far;
}

const Plane& Frustum::getLeft() const
{
    return _left;
}

const Plane& Frustum::getRight() const
{
    return _right;
}

const Plane& Frustum::getBottom() const
{
    return _bottom;
}

const Plane& Frustum::getTop() const
{
    return _top;
}

void Frustum::getMatrix(Matrix* dst) const
{
    M_ASSERT(dst);
    dst->set(_matrix);
}

void Frustum::getCorners(Vector3* corners) const
{
    getNearCorners(corners);
    getFarCorners(corners + 4);
}

void Frustum::getNearCorners(Vector3* corners) const
{
    M_ASSERT(corners);

    Plane::intersection(_near, _left, _top, &corners[0]);
    Plane::intersection(_near, _left, _bottom, &corners[1]);
    Plane::intersection(_near, _right, _bottom, &corners[2]);
    Plane::intersection(_near, _right, _top, &corners[3]);
}

void Frustum::frustumCorner(int corner, Vector3 & point) {
    float d1, d2, d3;
    float n1x, n1y, n1z, n2x, n2y, n2z, n3x, n3y, n3z;
    switch (corner) {
        case 0: // left, bottom, near
            n1x = this->_matrix.m[3] + this->_matrix.m[0]; n1y = this->_matrix.m[7] + this->_matrix.m[4]; n1z =this->_matrix.m[11] + this->_matrix.m[8] ; d1 = this->_matrix.m[15] + this->_matrix.m[12]; // left
            n2x = this->_matrix.m[3] + this->_matrix.m[1]; n2y = this->_matrix.m[7] + this->_matrix.m[5]; n2z =this->_matrix.m[11] + this->_matrix.m[9] ; d2 = this->_matrix.m[15] + this->_matrix.m[13]; // bottom
            n3x = this->_matrix.m[3] + this->_matrix.m[2]; n3y = this->_matrix.m[7] + this->_matrix.m[6]; n3z =this->_matrix.m[11] + this->_matrix.m[10]; d3 = this->_matrix.m[15] + this->_matrix.m[14]; // near
            break;
        case 1: // right, bottom, near
            n1x = this->_matrix.m[3] - this->_matrix.m[0]; n1y = this->_matrix.m[7] - this->_matrix.m[4]; n1z = this->_matrix.m[11] - this->_matrix.m[8] ; d1 = this->_matrix.m[15] - this->_matrix.m[12]; // right
            n2x = this->_matrix.m[3] + this->_matrix.m[1]; n2y = this->_matrix.m[7] + this->_matrix.m[5]; n2z = this->_matrix.m[11] + this->_matrix.m[9] ; d2 = this->_matrix.m[15] + this->_matrix.m[13]; // bottom
            n3x = this->_matrix.m[3] + this->_matrix.m[2]; n3y = this->_matrix.m[7] + this->_matrix.m[6]; n3z = this->_matrix.m[11] + this->_matrix.m[10]; d3 = this->_matrix.m[15] + this->_matrix.m[14]; // near
            break;
        case 2: // right, top, near
            n1x = this->_matrix.m[3] - this->_matrix.m[0]; n1y = this->_matrix.m[7] - this->_matrix.m[4]; n1z = this->_matrix.m[11] - this->_matrix.m[8] ; d1 = this->_matrix.m[15] - this->_matrix.m[12]; // right
            n2x = this->_matrix.m[3] - this->_matrix.m[1]; n2y = this->_matrix.m[7] - this->_matrix.m[5]; n2z = this->_matrix.m[11] - this->_matrix.m[9] ; d2 = this->_matrix.m[15] - this->_matrix.m[13]; // top
            n3x = this->_matrix.m[3] + this->_matrix.m[2]; n3y = this->_matrix.m[7] + this->_matrix.m[6]; n3z = this->_matrix.m[11] + this->_matrix.m[10]; d3 = this->_matrix.m[15] + this->_matrix.m[14]; // near
            break;
        case 3: // left, top, near
            n1x = this->_matrix.m[3] + this->_matrix.m[0]; n1y = this->_matrix.m[7] + this->_matrix.m[4]; n1z =this->_matrix.m[11] + this->_matrix.m[8] ; d1 = this->_matrix.m[15] + this->_matrix.m[12]; // left
            n2x = this->_matrix.m[3] - this->_matrix.m[1]; n2y = this->_matrix.m[7] - this->_matrix.m[5]; n2z =this->_matrix.m[11] - this->_matrix.m[9] ; d2 = this->_matrix.m[15] - this->_matrix.m[13]; // top
            n3x = this->_matrix.m[3] + this->_matrix.m[2]; n3y = this->_matrix.m[7] + this->_matrix.m[6]; n3z =this->_matrix.m[11] + this->_matrix.m[10]; d3 = this->_matrix.m[15] + this->_matrix.m[14]; // near
            break;
        case 4: // right, bottom, far
            n1x = this->_matrix.m[3] - this->_matrix.m[0]; n1y = this->_matrix.m[7] - this->_matrix.m[4]; n1z = this->_matrix.m[11] - this->_matrix.m[8] ; d1 = this->_matrix.m[15] - this->_matrix.m[12]; // right
            n2x = this->_matrix.m[3] + this->_matrix.m[1]; n2y = this->_matrix.m[7] + this->_matrix.m[5]; n2z = this->_matrix.m[11] + this->_matrix.m[9] ; d2 = this->_matrix.m[15] + this->_matrix.m[13]; // bottom
            n3x = this->_matrix.m[3] - this->_matrix.m[2]; n3y = this->_matrix.m[7] - this->_matrix.m[6]; n3z = this->_matrix.m[11] - this->_matrix.m[10]; d3 = this->_matrix.m[15] - this->_matrix.m[14]; // far
            break;
        case 5: // left, bottom, far
            n1x = this->_matrix.m[3] + this->_matrix.m[0]; n1y = this->_matrix.m[7] + this->_matrix.m[4]; n1z = this->_matrix.m[11] + this->_matrix.m[8] ; d1 = this->_matrix.m[15] + this->_matrix.m[12]; // left
            n2x = this->_matrix.m[3] + this->_matrix.m[1]; n2y = this->_matrix.m[7] + this->_matrix.m[5]; n2z = this->_matrix.m[11] + this->_matrix.m[9] ; d2 = this->_matrix.m[15] + this->_matrix.m[13]; // bottom
            n3x = this->_matrix.m[3] - this->_matrix.m[2]; n3y = this->_matrix.m[7] - this->_matrix.m[6]; n3z = this->_matrix.m[11] - this->_matrix.m[10]; d3 = this->_matrix.m[15] - this->_matrix.m[14]; // far
            break;
        case 6: // left, top, far
            n1x = this->_matrix.m[3] + this->_matrix.m[0]; n1y = this->_matrix.m[7] + this->_matrix.m[4]; n1z = this->_matrix.m[11] + this->_matrix.m[8] ; d1 = this->_matrix.m[15] + this->_matrix.m[12]; // left
            n2x = this->_matrix.m[3] - this->_matrix.m[1]; n2y = this->_matrix.m[7] - this->_matrix.m[5]; n2z = this->_matrix.m[11] - this->_matrix.m[9] ; d2 = this->_matrix.m[15] - this->_matrix.m[13]; // top
            n3x = this->_matrix.m[3] - this->_matrix.m[2]; n3y = this->_matrix.m[7] - this->_matrix.m[6]; n3z = this->_matrix.m[11] - this->_matrix.m[10]; d3 = this->_matrix.m[15] - this->_matrix.m[14]; // far
            break;
        case 7: // right, top, far
            n1x = this->_matrix.m[3] - this->_matrix.m[0]; n1y = this->_matrix.m[7] - this->_matrix.m[4]; n1z = this->_matrix.m[11] - this->_matrix.m[8] ; d1 = this->_matrix.m[15] - this->_matrix.m[12]; // right
            n2x = this->_matrix.m[3] - this->_matrix.m[1]; n2y = this->_matrix.m[7] - this->_matrix.m[5]; n2z = this->_matrix.m[11] - this->_matrix.m[9] ; d2 = this->_matrix.m[15] - this->_matrix.m[13]; // top
            n3x = this->_matrix.m[3] - this->_matrix.m[2]; n3y = this->_matrix.m[7] - this->_matrix.m[6]; n3z = this->_matrix.m[11] - this->_matrix.m[10]; d3 = this->_matrix.m[15] - this->_matrix.m[14]; // far
            break;
//            default:
    }

    Vector3 v1(n2x,n2y,n2z);
    Vector3 v2(n3x,n3y,n3z);
    Vector3 v3(n1x,n1y,n1z);


    Vector3 cross23;
    Vector3::cross(v1,Vector3(n3x,n3y,n3z),&cross23);

    Vector3 cross31;
    Vector3::cross(v2, Vector3(n1x,n1y,n1z),&cross31);

    Vector3 cross12;
    Vector3::cross(v3, Vector3(n2x,n2y,n2z),&cross12);

    float invDot = 1.0f / (n1x * cross23.x + n1y * cross23.y + n1z * cross23.z);
    point.x = (-cross23.x * d1 - cross31.x * d2 - cross12.x * d3) * invDot;
    point.y = (-cross23.y * d1 - cross31.y * d2 - cross12.y * d3) * invDot;
    point.z = (-cross23.z * d1 - cross31.z * d2 - cross12.z * d3) * invDot;
}

void Frustum::getFarCorners(Vector3* corners) const
{
    M_ASSERT(corners);

    Plane::intersection(_far, _right, _top, &corners[0]);
    Plane::intersection(_far, _right, _bottom, &corners[1]);
    Plane::intersection(_far, _left, _bottom, &corners[2]);
    Plane::intersection(_far, _left, _top, &corners[3]);
}

bool Frustum::intersects(const Vector3& point) const
{
    if (_near.distance(point) <= 0)
        return false;
    if (_far.distance(point) <= 0)
        return false;
    if (_left.distance(point) <= 0)
        return false;
    if (_right.distance(point) <= 0)
        return false;
    if (_top.distance(point) <= 0)
        return false;
    if (_bottom.distance(point) <= 0)
        return false;

    return true;
}

bool Frustum::intersects(float x, float y, float z) const
{
    return intersects(Vector3(x, y, z));
}

bool Frustum::intersects(const BoundingSphere& sphere) const
{
    return sphere.intersects(*this);
}

bool Frustum::intersects(const BoundingBox& box) const
{
    return box.intersects(*this);
}

float Frustum::intersects(const Plane& plane) const
{
    return plane.intersects(*this);
}

float Frustum::intersects(const Ray& ray) const
{
    return ray.intersects(*this);
}

void Frustum::set(const Frustum& frustum)
{
    _near = frustum._near;
    _far = frustum._far;
    _bottom = frustum._bottom;
    _top = frustum._top;
    _left = frustum._left;
    _right = frustum._right;
    _matrix.set(frustum._matrix);
}

void Frustum::updatePlanes()
{
    _near.set(Vector3(_matrix.m[3] + _matrix.m[2], _matrix.m[7] + _matrix.m[6], _matrix.m[11] + _matrix.m[10]), _matrix.m[15] + _matrix.m[14]);
    _far.set(Vector3(_matrix.m[3] - _matrix.m[2], _matrix.m[7] - _matrix.m[6], _matrix.m[11] - _matrix.m[10]), _matrix.m[15] - _matrix.m[14]);
    _bottom.set(Vector3(_matrix.m[3] + _matrix.m[1], _matrix.m[7] + _matrix.m[5], _matrix.m[11] + _matrix.m[9]), _matrix.m[15] + _matrix.m[13]);
    _top.set(Vector3(_matrix.m[3] - _matrix.m[1], _matrix.m[7] - _matrix.m[5], _matrix.m[11] - _matrix.m[9]), _matrix.m[15] - _matrix.m[13]);
    _left.set(Vector3(_matrix.m[3] + _matrix.m[0], _matrix.m[7] + _matrix.m[4], _matrix.m[11] + _matrix.m[8]), _matrix.m[15] + _matrix.m[12]);
    _right.set(Vector3(_matrix.m[3] - _matrix.m[0], _matrix.m[7] - _matrix.m[4], _matrix.m[11] - _matrix.m[8]), _matrix.m[15] - _matrix.m[12]);

    for(int i = 0;i<8; i++){
        frustumCorner(i,corners[i]);
    }
}

void Frustum::set(const Matrix& matrix)
{
    _matrix.set(matrix);

    // Update the planes.
    updatePlanes();
}

