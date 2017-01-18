#ifndef MATH_H
#define MATH_H

#include "General/3DTypes.h"

/*
 * General purpose math functions
 */
namespace Alamo
{
unsigned int RoundToPowerOf2(unsigned int val);
float        GetRandom(float x, float y);
int          GetRandom(int   x, int   y);

float clamp(float val, float min, float max);
float saturate(float val);
Quaternion slerp(const Quaternion& q1, const Quaternion& q2, float s);

// Linear interpolation of x and y, based on s
template <typename T>
static T lerp(const T& x, const T& y, float s)
{
    return s * y + (1 - s) * x;
}

// Cubic interpolation of x and y, based on s
template <typename T>
static T cubic(const T& x, const T& y, float s)
{
    return x * (2*s*s*s - 3*s*s + 1) + y * (3*s*s - 2*s*s*s);
}

class BezierCurve
{
    Vector2 m_p0, m_p1, m_p2, m_p3;
public:
    float   length(float s = 1.0f) const;
    Vector2 interpolate(float s) const;
    Vector2 derivative(float s) const;
    float   parameterize(float x) const;

    BezierCurve(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3);
};

}
#endif