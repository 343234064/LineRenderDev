#include "Utils.h"


using namespace std;

Float3 CalculateNormal(Float3& x, Float3& y, Float3& z)
{
	Float3 U = y - x;
	Float3 V = z - x;

	Float3 Normal = Float3(0.0f, 0.0f, 0.0f);

	Normal = Cross(U, V);
	Normal = Normalize(Normal);

	return Normal;
}

Float3 operator-(const Float3& a, const Float3& b)
{
	return Float3(a.x - b.x, a.y - b.y, a.z - b.z);
}

Float3 operator+(const Float3& a, const Float3& b)
{
	return Float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Float3 operator*(const Float3& a, const double b)
{
	return Float3(a.x * b, a.y * b, a.z * b);
}

Float3 operator*(const Float3& a, const Float3& b)
{
	return Float3(a.x * b.x, a.y * b.y, a.z * b.z);
}

Float3 operator/(const Float3& a, const double b)
{
	return Float3(a.x / b, a.y / b, a.z / b);
}

Float3 Normalize(const Float3& a)
{
	float MagInv = 1.0f / Length(a);
	return a * MagInv;
}

float Length(const Float3& a)
{
	return MAX(0.000001f, sqrt(a.x * a.x + a.y * a.y + a.z * a.z));
}

Float3 Cross(const Float3& a, const Float3& b)
{
	return Float3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	);
}

float Dot(const Float3& a, const Float3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

