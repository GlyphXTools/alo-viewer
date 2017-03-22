#ifndef _3DTYPES_H
#define _3DTYPES_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#ifndef NDEBUG
#define D3D_DEBUG_INFO
#endif
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr.h>

/*
 * Basic 3D types for 3D math
 * - Vector3
 * - Vector4
 * - Matrix
 *
 * Some properties:
 * - Matrices and vectors behave like in Direct3D
 * - Matrices have translation in bottom row
 * - Vectors are column vectors
 * - Matrix-vector multiplication is post-multiplication
 */
namespace Alamo
{

struct Matrix;
struct Vector3;
struct Vector4;

static const float PI = D3DX_PI;

static float ToRadians(float deg) { return D3DXToRadian(deg); }
static float ToDegrees(float rad) { return D3DXToDegree(rad); }

struct Point
{
    int x;
    int y;

    Point(int _x, int _y) : x(_x), y(_y) {}
    Point() {}
};

struct Quaternion : public D3DXQUATERNION
{
	Quaternion& operator +=(const Quaternion& q)		{ *(D3DXQUATERNION*)this += q; return *this; }
	Quaternion& operator *=(const Quaternion& q)		{ *(D3DXQUATERNION*)this *= q; return *this; }
	Quaternion  operator + (const Quaternion& q) const	{ return *(const D3DXQUATERNION*)this + q; }
    Quaternion  operator * (const Quaternion& q) const  { return *(const D3DXQUATERNION*)this * q; }

    Quaternion slerp(const Quaternion& q, float s) const;

	Quaternion(float x, float y, float z, float w) : D3DXQUATERNION(x,y,z,w) {}
    Quaternion(const D3DXQUATERNION& quat) : D3DXQUATERNION(quat) {}
	Quaternion(const Vector3& axis, float angle);
    Quaternion(float x, float y, float z);
	Quaternion() {}
};

struct Vector3 : public D3DXVECTOR3
{
    float   tilt()   const { return atan2f(z, sqrtf(x*x + y*y)); }
    float   zAngle() const { return atan2f(y, x); }
    void    normalize()    { D3DXVec3Normalize(this, this); }
    float   length() const { return D3DXVec3Length(this); }
    float   dot  (const Vector3& v) const { return D3DXVec3Dot(this, &v); }
	Vector3 cross(const Vector3& v) const { Vector3 out; D3DXVec3Cross(&out, this, &v); return out; }
	
    Vector3  operator- ()                 const { return -*(const D3DXVECTOR3*)this; }
    Vector3  operator+ (const Vector3& v) const	{ return *(const D3DXVECTOR3*)this + v; }
    Vector3  operator- (const Vector3& v) const	{ return *(const D3DXVECTOR3*)this - v; }
    Vector3  operator* (const Vector3& v) const	{ return Vector3(x * v.x, y * v.y, z * v.z); }
    Vector3  operator+ (float s)          const	{ return Vector3(x + s, y + s, z + s); }
    Vector3  operator- (float s)          const	{ return Vector3(x - s, y - s, z - s); }
    Vector3  operator* (float s)          const { return *(const D3DXVECTOR3*)this * s; }
    Vector3  operator/ (float s)          const { return *(const D3DXVECTOR3*)this / s; }
    Vector3& operator+=(const Vector3& v)		{ *(D3DXVECTOR3*)this += v; return *this; }
	Vector3& operator-=(const Vector3& v)		{ *(D3DXVECTOR3*)this -= v; return *this; }
	Vector3& operator*=(const Vector3& v)		{ x *= v.x; y *= v.y; z *= v.z; return *this; }
	Vector3& operator+=(float s)		        { x += s; y += s; z += s; return *this; }
	Vector3& operator-=(float s)		        { x -= s; y -= s; z -= s; return *this; }
	Vector3& operator*=(float s)				{ *(D3DXVECTOR3*)this *= s; return *this; }
	Vector3& operator/=(float s)				{ *(D3DXVECTOR3*)this /= s; return *this; }

	Vector3  operator* (const Matrix& m) const;
	Vector3& operator*=(const Matrix& m);

	Vector3(float x, float y, float z) : D3DXVECTOR3(x,y,z) {}
    Vector3(const float* f) : D3DXVECTOR3(f[0], f[1], f[2]) {}
    Vector3(const D3DXVECTOR4& vec) : D3DXVECTOR3(vec.x, vec.y, vec.z) {}
    Vector3(const D3DXVECTOR3& vec) : D3DXVECTOR3(vec) {}
    Vector3(const D3DXVECTOR2& vec, float z) : D3DXVECTOR3(vec.x, vec.y, z) {}
    Vector3(float zAngle, float tilt);
	Vector3() {}
};

struct Vector4 : public D3DXVECTOR4
{
    void    normalize()    { D3DXVec4Normalize(this, this); }
    float   length() const { return D3DXVec4Length(this); }
    float   dot(const Vector4& v) const { return D3DXVec4Dot(this, &v); }
	
    Vector4  operator- ()                 const { return -*(const D3DXVECTOR4*)this; }
    Vector4  operator+ (const Vector4& v) const	{ return *(const D3DXVECTOR4*)this + v; }
    Vector4  operator- (const Vector4& v) const	{ return *(const D3DXVECTOR4*)this - v; }
    Vector4  operator* (const Vector4& v) const	{ return Vector4(x * v.x, y * v.y, z * v.z, w * v.w); }
    Vector4  operator* (float s)          const { return *(const D3DXVECTOR4*)this * s; }
    Vector4  operator/ (float s)          const { return *(const D3DXVECTOR4*)this / s; }
    Vector4& operator+=(const Vector4& v)		{ *(D3DXVECTOR4*)this += v; return *this; }
	Vector4& operator-=(const Vector4& v)		{ *(D3DXVECTOR4*)this -= v; return *this; }
	Vector4& operator*=(const Vector4& v)		{ x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
	Vector4& operator*=(float s)				{ *(D3DXVECTOR4*)this *= s; return *this; }
	Vector4& operator/=(float s)				{ *(D3DXVECTOR4*)this /= s; return *this; }

	Vector4  operator* (const Matrix& m) const;
	Vector4& operator*=(const Matrix& m);

    Vector4(float x, float y, float z, float w) : D3DXVECTOR4(x,y,z,w) {}
    Vector4(const float* f) : D3DXVECTOR4(f[0], f[1], f[2], f[3]) {}
    Vector4(const D3DXVECTOR4& vec) : D3DXVECTOR4(vec) {}
    Vector4(const D3DXCOLOR& c) : D3DXVECTOR4(c.r, c.g, c.b, c.a) {}
    Vector4(const D3DXVECTOR3& vec, float w) : D3DXVECTOR4(vec.x, vec.y, vec.z, w) {}
	Vector4() {}
};

struct Matrix : public D3DXMATRIX
{
	static const Matrix Identity;

    // Natural matrix operations
    Matrix  operator* (const Matrix& mat) const { return *(const D3DXMATRIX*)this * mat;   }
	Matrix  operator+ (const Matrix& mat) const { return *(const D3DXMATRIX*)this + mat;   }
	Matrix  operator- (const Matrix& mat) const { return *(const D3DXMATRIX*)this - mat;   }
    Matrix  operator* (float scale)       const { return *(const D3DXMATRIX*)this * scale; }
	Matrix  operator/ (float scale)       const { return *(const D3DXMATRIX*)this / scale; }
	Matrix& operator*=(const Matrix& mat)       { *(D3DXMATRIX*)this *= mat;   return *this; }
	Matrix& operator+=(const Matrix& mat)       { *(D3DXMATRIX*)this += mat;   return *this; }
	Matrix& operator-=(const Matrix& mat)       { *(D3DXMATRIX*)this -= mat;   return *this; }
    Matrix& operator*=(float scale)             { *(D3DXMATRIX*)this *= scale; return *this; }
	Matrix& operator/=(float scale)             { *(D3DXMATRIX*)this /= scale; return *this; }

	void  invert();
	void  transpose();
	float determinant() const;
    
    Matrix inverse() const;

    Vector4 column(int col)    const { return Vector4((*this)(0, col), (*this)(1, col), (*this)(2, col), (*this)(3, col)); }
    Vector4 row(int row)       const { return Vector4((*this)(row, 0), (*this)(row, 1), (*this)(row, 2), (*this)(row, 3)); }
    Vector3 getTranslation()   const { return Vector3(_41, _42, _43); }
    Matrix  getRotationScale() const;

    void setTranslation(const Vector3& v) { _41  = v.x; _42  = v.y; _43  = v.z; }
    void addTranslation(const Vector3& v) { _41 += v.x; _42 += v.y; _43 += v.z; }

	// Apply operations
	Vector4 operator*(const Vector4& v) const;
	Vector3 operator*(const Vector3& v) const;

    void decompose(Vector3* scale, Quaternion* rotation, Vector3* translation);

    Matrix(float _11, float _12, float _13, float _14, float _21, float _22, float _23, float _24,
	        float _31, float _32, float _33, float _34, float _41, float _42, float _43, float _44)
        : D3DXMATRIX(_11, _12, _13, _14, _21, _22, _23, _24, _31, _32, _33, _34, _41, _42, _43, _44) {}
	Matrix(float cells[16]) : D3DXMATRIX(cells) {}
    Matrix(const D3DXMATRIX& mat) : D3DXMATRIX(mat) {}
	Matrix(const Quaternion& q);
    Matrix(const Vector3& scale, const Quaternion& rotation, const Vector3& translation);
	Matrix() {}
};

struct Vector2 : public D3DXVECTOR2
{
    void    normalize()    { D3DXVec2Normalize(this, this); }
    float   length() const { return D3DXVec2Length(this); }
    float   dot(const Vector2& v) const { return D3DXVec2Dot(this, &v); }
	
    Vector2  operator- ()                 const { return -*(const D3DXVECTOR2*)this; }
    Vector2  operator+ (const Vector2& v) const	{ return *(const D3DXVECTOR2*)this + v; }
    Vector2  operator- (const Vector2& v) const	{ return *(const D3DXVECTOR2*)this - v; }
    Vector2  operator* (const Vector2& v) const	{ return Vector2(x * v.x, y * v.y); }
    Vector2  operator* (float s)          const { return *(const D3DXVECTOR2*)this * s; }
    Vector2  operator/ (float s)          const { return *(const D3DXVECTOR2*)this / s; }
    Vector2& operator+=(const Vector2& v)		{ *(D3DXVECTOR2*)this += v; return *this; }
	Vector2& operator-=(const Vector2& v)		{ *(D3DXVECTOR2*)this -= v; return *this; }
	Vector2& operator*=(const Vector2& v)		{ x *= v.x; y *= v.y;       return *this; }
	Vector2& operator*=(float s)				{ *(D3DXVECTOR2*)this *= s; return *this; }
	Vector2& operator/=(float s)				{ *(D3DXVECTOR2*)this /= s; return *this; }

	Vector2(float x, float y) : D3DXVECTOR2(x,y) {}
    Vector2(float angle);
    Vector2(const float* f) : D3DXVECTOR2(f[0], f[1]) {}
    Vector2(const D3DXVECTOR2& vec) : D3DXVECTOR2(vec) {}
	Vector2() {}
};

Vector4 normalize(const Vector4& v);
Vector3 normalize(const Vector3& v);
Vector2 normalize(const Vector2& v);
float inline dot(const Vector4& v1, const Vector4& v2) { return v1.dot(v2); }
float inline dot(const Vector3& v1, const Vector3& v2) { return v1.dot(v2); }
float inline dot(const Vector2& v1, const Vector2& v2) { return v1.dot(v2); }
Vector3 inline cross(const Vector3& v1, const Vector3& v2) { return v1.cross(v2); }
Matrix transpose(const Matrix& m);

struct Color : public D3DXCOLOR
{
    Color  operator* (const D3DXCOLOR& c) const { return Color(r*c.r, g*c.g, b*c.b, a*c.a); }
    Color  operator* (float s)            const { return Color(r*s, g*s, b*s, a*s); }
    Color& operator*=(const D3DXCOLOR& c)       { r*=c.r; g*=c.g; b*=c.b; a*=c.a; return *this; }
    Color& operator*=(float s)                  { r*=s; g*=s; b*=s; a*=s; return *this; }

    Color ToHSV() const;
    Color ToRGB() const;

    Color(float r, float g, float b, float a) : D3DXCOLOR(r,g,b,a) {}
    Color(const Vector4& v) : D3DXCOLOR(v.x,v.y,v.z,v.w) {}
    Color(const D3DXCOLOR& c) : D3DXCOLOR(c) {}
    Color(const D3DXCOLOR& c, float a) : D3DXCOLOR(c.r, c.g, c.b, a) {}
	Color() {}
};

// Constructs a right-handed view matrix
Matrix CreateViewMatrix(const Vector3& eye, const Vector3& at, const Vector3& up);

// Constructs a right-handed perspective matrix
Matrix CreatePerspectiveMatrix(float fovY, float aspect, float zn, float zf);

}

#endif