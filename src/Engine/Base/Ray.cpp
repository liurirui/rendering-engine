#include "Base.h"
#include "Ray.h"
#include "Plane.h"
#include "Frustum.h"
#include "BoundingSphere.h"
#include "BoundingBox.h"
#include <float.h>


Ray::Ray()
    : _direction(0, 0, 1)
{
}

Ray::Ray(const Vector3& origin, const Vector3& direction)
{
    set(origin, direction);
}

Ray::Ray(float originX, float originY, float originZ, float dirX, float dirY, float dirZ)
{
    set(Vector3(originX, originY, originZ), Vector3(dirX, dirY, dirZ));
}

Ray::Ray(const Ray& copy)
{
    set(copy);
}

Ray::~Ray()
{
}

const Vector3& Ray::getOrigin() const
{
    return _origin;
}

void Ray::setOrigin(const Vector3& origin)
{
    _origin = origin;
}

void Ray::setOrigin(float x, float y, float z)
{
    _origin.set(x, y, z);
}

const Vector3& Ray::getDirection() const
{
    return _direction;
}

void Ray::setDirection(const Vector3& direction)
{
    _direction = direction;
    normalize();
}

void Ray::setDirection(float x, float y, float z)
{
    _direction.set(x, y, z);
    normalize();
}

float Ray::intersects(const BoundingSphere& sphere) const
{
    return sphere.intersects(*this);
}

float Ray::intersects(const BoundingBox& box) const
{
    return box.intersects(*this);
}

float Ray::intersects(const Frustum& frustum) const
{
    Plane n = frustum.getNear();
    float nD = intersects(n);
    float nOD = n.distance(_origin);

    Plane f = frustum.getFar();
    float fD = intersects(f);
    float fOD = f.distance(_origin);

    Plane l = frustum.getLeft();
    float lD = intersects(l);
    float lOD = l.distance(_origin);

    Plane r = frustum.getRight();
    float rD = intersects(r);
    float rOD = r.distance(_origin);

    Plane b = frustum.getBottom();
    float bD = intersects(b);
    float bOD = b.distance(_origin);

    Plane t = frustum.getTop();
    float tD = intersects(t);
    float tOD = t.distance(_origin);

    // If the ray's origin is in the negative half-space of one of the frustum's planes
    // and it does not intersect that same plane, then it does not intersect the frustum.
    if ((nOD < 0.0f && nD < 0.0f) || (fOD < 0.0f && fD < 0.0f) ||
        (lOD < 0.0f && lD < 0.0f)  || (rOD < 0.0f && rD < 0.0f) ||
        (bOD < 0.0f && bD < 0.0f)  || (tOD < 0.0f && tD < 0.0f))
    {
        return Ray::INTERSECTS_NONE;
    }

    // Otherwise, the intersection distance is the minimum positive intersection distance.
    float d = (nD > 0.0f) ? nD : 0.0f;
    d = (fD > 0.0f) ? ((d == 0.0f) ? fD : min(fD, d)) : d;
    d = (lD > 0.0f) ? ((d == 0.0f) ? lD : min(lD, d)) : d;
    d = (rD > 0.0f) ? ((d == 0.0f) ? rD : min(rD, d)) : d;
    d = (tD > 0.0f) ? ((d == 0.0f) ? bD : min(bD, d)) : d;
    d = (bD > 0.0f) ? ((d == 0.0f) ? tD : min(tD, d)) : d;

    return d;
}

float Ray::intersects(const Plane& plane) const
{
    const Vector3& normal = plane.getNormal();
    // If the origin of the ray is on the plane then the distance is zero.
    float alpha = (normal.dot(_origin) + plane.getDistance());
    if (fabs(alpha) < MATH_EPSILON)
    {
        return 0.0f;
    }

    float dot = normal.dot(_direction);
    
    // If the dot product of the plane's normal and this ray's direction is zero,
    // then the ray is parallel to the plane and does not intersect it.
    if (dot == 0.0f)
    {
        return INTERSECTS_NONE;
    }
    
    // Calculate the distance along the ray's direction vector to the point where
    // the ray intersects the plane (if it is negative the plane is behind the ray).
    float d = -alpha / dot;
    if (d < 0.0f)
    {
        return INTERSECTS_NONE;
    }
    return d;
}

void Ray::set(const Vector3& origin, const Vector3& direction)
{
    _origin = origin;
    _direction = direction;
    normalize();
}

void Ray::set(const Ray& ray)
{
    _origin = ray._origin;
    _direction = ray._direction;
    normalize();
}

void Ray::transform(const Matrix& matrix)
{
    matrix.transformPoint(&_origin);
    matrix.transformVector(&_direction);
    _direction.normalize();
}

void Ray::normalize()
{
    if (_direction.isZero())
    {
        //LOGE("Invalid ray object; a ray's direction must be non-zero.");
        return;
    }

    // Normalize the ray's direction vector.
    float normalizeFactor = 1.0f / sqrt(_direction.x * _direction.x + _direction.y * _direction.y + _direction.z * _direction.z);
    if (normalizeFactor != 1.0f)
    {
        _direction.x *= normalizeFactor;
        _direction.y *= normalizeFactor;
        _direction.z *= normalizeFactor;
    }
}

    bool Ray::rayIntersectsTriangle(const Vector3 &v0, const Vector3 &v1, const Vector3 &v2,
                                      float &t) {
        float u, v;

        Vector3 e1 = v1 - v0;
        Vector3 e2 = v2 - v0;

        Vector3 e_normal;
        Vector3::cross(e1,e2, &e_normal);

        float ddN = this->_direction.dot(e_normal);
        float sign = 1;
        if(ddN > 0) {
            sign = 1;
        } else {
            sign = -1;
            ddN = -ddN;
        }

        Vector3 h;
        Vector3::cross(this->_direction,e2, &h);
        float det = sign * e1.dot(h);

        // if determinant is near zero, ray lies in plane of triangle
        if (det > -0.00001 && det < 0.00001)
            return (false);
        float inv_det = 1.0f / det;

        // calculate u parameter and test bounds
        Vector3 s = this->_origin - v0;
        u = sign * inv_det * s.dot(h);

        if (u < 0.0 || u > 1.0)
            return (false);

        // calculate v parameter and test bounds
        Vector3 q;
        Vector3::cross(s, e1, &q);
        v = sign *  inv_det * this->_direction.dot(q);
        if (v < 0.0 || u + v > 1.0)
            return (false);

        // at this stage we can compute t to find out where
        // the intersection point is on the line
        t = sign * inv_det * e2.dot(q);
        if (t > 0)  // ray intersection
            return (true);
        else  // this means that there is a line intersection
            // but not a ray intersection
            return (false);
    }

    bool Ray::intersectsTriangle(const Vector3 &p1,
                                   const Vector3 &p2, const Vector3 &p3,
                                   Vector3 &target, float &dis) {
//        Vector3 _direction = dir;

        float dir_norm = this->_direction.length();
        if (dir_norm < FLT_EPSILON)
            return false;
//        _direction.normalize();
        bool ret = rayIntersectsTriangle(p1, p2, p3, dis);

        if (ret) {
            if (dis > dir_norm) {
                target = this->_origin + this->_direction * dis ;
            } else {
                ret = false;
            }
        }

//        if (ret)
//        {
//            btVector3 n = (p3 - p1).cross(p2 - p1);
//            n.safeNormalize();
//            if (n.dot(dir) < 0)
//                normal = n;
//            else
//                normal = -n;
//        }
        return ret;
    }

