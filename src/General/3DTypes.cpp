#include "3dtypes.h"
#include "General/Exceptions.h"
#include <cmath>
#include <algorithm>
using namespace std;

namespace Alamo
{

//
// Color
//
Color Color::ToHSV() const
{
	float min   = min(min(r, g), b);
	float max   = max(max(r, g), b);
    float delta = max - min;
	if (delta != 0 && max != 0)
    {
	    float f = (r == max) ? (g - b) / delta : ((g == max) ? (b - r) : (r - g));
	    int   i = (r == max) ? 0 : ((g == max) ? 2 : 4);
	    return Color( fmod((i + f / delta) / 6 + 1.0f, 1.0f), delta / max, max, a);
    }
    return Color(0.0, 0.0, max, a);
}

Color Color::ToRGB() const
{
    if (r != -1)
    {
        int   i = (int)(r * 6) % 6;
        float f = r * 6 - i;
        float p = b * (1 - g);
        float q = b * (1 - f * g);
        float t = b * (1 - (1 - f) * g);
        switch (i) {
	        case 0: return Color(b, t, p, a);
	        case 1: return Color(q, b, p, a);
	        case 2: return Color(p, b, t, a);
	        case 3: return Color(p, q, b, a);
	        case 4: return Color(t, p, b, a);
	        case 5: return Color(b, p, q, a);
        }
    }
    return Color(b,b,b,a);
}

//
// Vector2 
//
Vector2::Vector2(float angle)
    : D3DXVECTOR2(cos(angle), sin(angle))
{
}

//
// Vector3
//
Vector3  Vector3::operator* (const Matrix& m) const { return m * *this; }
Vector3& Vector3::operator*=(const Matrix& m)       { return *this = m * *this; }

Vector3::Vector3(float zAngle, float tilt)
{
    x = cos(zAngle) * cos(tilt);
    y = sin(zAngle) * cos(tilt);
    z = sin(tilt);
}

//
// Vector4
//
Vector4  Vector4::operator* (const Matrix& m) const	{ return m * *this; }
Vector4& Vector4::operator*=(const Matrix& m)       { return *this = m * *this; }

//
// Quaternion
//
Quaternion::Quaternion(const Vector3& axis, float angle)
{
    D3DXQuaternionRotationAxis(this, &axis, angle);
}

Quaternion::Quaternion(float x, float y, float z)
{
    D3DXQuaternionRotationYawPitchRoll(this, y, x, z);
}

Quaternion Quaternion::slerp(const Quaternion& q, float s) const
{
    Quaternion res;
    D3DXQuaternionSlerp(&res, this, &q, s);
    return res;
}

//
// Matrix
//
const Matrix Matrix::Identity(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);

Matrix::Matrix(const Quaternion& q)
{
    D3DXMatrixRotationQuaternion(this, &q);
}

Matrix Matrix::inverse() const
{
    Matrix result;
    D3DXMatrixInverse(&result, NULL, this);
    return result;
}

void Matrix::invert()
{
    D3DXMatrixInverse(this, NULL, this);
}

void Matrix::transpose()
{
    D3DXMatrixTranspose(this, this);
}

Vector4 Matrix::operator*(const Vector4& v) const
{
    Vector4 out;
    D3DXVec4Transform(&out, &v, this);
    return out;
}

Vector3 Matrix::operator*(const Vector3& v) const
{
    Vector3 out;
    D3DXVec3TransformCoord(&out, &v, this);
    return out;
}

Matrix Matrix::getRotationScale() const
{
    return Matrix(
        _11, _12, _13, _14,
        _21, _22, _23, _24,
        _31, _32, _33, _34,
          0,   0,   0, _44 );
}

Matrix transpose(const Matrix& m)
{
    Matrix out;
    D3DXMatrixTranspose(&out, &m);
    return out;
}

void Matrix::decompose(Vector3* scale, Quaternion* rotation, Vector3* translation)
{
    D3DXMatrixDecompose(scale, rotation, translation, this);
}

Matrix::Matrix(const Vector3& scale, const Quaternion& rotation, const Vector3& translation)
{
    D3DXMatrixTransformation(this, NULL, NULL, &scale, NULL, &rotation, &translation);
}

Vector4 normalize(const Vector4& v) { Vector4 out(v); out.normalize(); return out; }
Vector3 normalize(const Vector3& v) { Vector3 out(v); out.normalize(); return out; }
Vector2 normalize(const Vector2& v) { Vector2 out(v); out.normalize(); return out; }

// Constructs a right-handed view matrix
// Taken from the DirectX manual
Matrix CreateViewMatrix(const Vector3& eye, const Vector3& at, const Vector3& up)
{
    Matrix out;
    D3DXMatrixLookAtRH(&out, &eye, &at, &up);
    return out;
}

Matrix CreatePerspectiveMatrix(float fovY, float aspect, float zn, float zf)
{
    Matrix out;
    D3DXMatrixPerspectiveFovRH(&out, fovY, aspect, zn, zf);
    return out;
}

}