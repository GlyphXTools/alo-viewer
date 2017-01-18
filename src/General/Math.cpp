#include "General/Math.h"

namespace Alamo {

unsigned int RoundToPowerOf2(unsigned int val)
{
    unsigned int result = 1;
    while (result < val)
    {
        result *= 2;
    }
    return result;
}

float GetRandom(float x, float y)
{
    return (y - x) * (rand() / (RAND_MAX + 1.0f)) + x;
}

int GetRandom(int x, int y)
{
    return (y - x) * rand() / (RAND_MAX + 1) + x;
}

float clamp(float val, float min, float max)
{
    return (val < min) ? min : (val > max ? max : val);
}

float saturate(float val)
{
    return clamp(val, 0.0f, 1.0f);
}

Quaternion slerp(const Quaternion& q1, const Quaternion& q2, float s)
{
    return q1.slerp(q2, s);
}

//
// BezierCurve
//
float BezierCurve::parameterize(float x) const
{
    static const int NUM_ITERATIONS = 10;

    // Return (approximate) s, such that length(s) == x
    // We use a 'binary' search approach for a certain number of iterations
    float s = 0.5f;
    float w = 0.25f;
    for (int i = 0; i < NUM_ITERATIONS; i++, w /= 2)
    {
        if (x < length(s)) s -= w;
        else               s += w;
    }
    return s;
}

float BezierCurve::length(float s) const
{
    // Use Simpson's rule to estimate the integration of the derivative length over [0,s]
    return (s / 6) * (derivative(0).length() + 4 * derivative(s / 2).length() + derivative(s).length());
}

Vector2 BezierCurve::interpolate(float s) const
{
    // Interpolate the spline
    return  m_p0 * (1-s)*(1-s)*(1-s) +
        3 * m_p1 * s*(1-s)*(1-s) +
        3 * m_p2 * s*s*(1-s)+
            m_p3 * s*s*s;
}

Vector2 BezierCurve::derivative(float s) const
{
    // Interpolate the derivative of the spline
    return 3 * (m_p3 + 3*m_p1 - 3*m_p2 - m_p0) * s * s
         + 6 * (m_p0 + m_p2 - 2 * m_p1) * s
         + 3 * (m_p1 - m_p0);
}

BezierCurve::BezierCurve(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3)
    : m_p0(p0), m_p1(p1), m_p2(p2), m_p3(p3)
{
}

}