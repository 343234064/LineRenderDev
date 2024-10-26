#pragma once

#include <iostream>
#include "others\float16_t.hpp"

typedef unsigned char Byte;
typedef unsigned int uint;

#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))


//Not commutative
inline unsigned int HashCombine(unsigned int A, unsigned int C)
{
	unsigned int B = 0x9e3779b9;
	A += B;

	A -= B; A -= C; A ^= (C >> 13);
	B -= C; B -= A; B ^= (A << 8);
	C -= A; C -= B; C ^= (B >> 13);
	A -= B; A -= C; A ^= (C >> 12);
	B -= C; B -= A; B ^= (A << 16);
	C -= A; C -= B; C ^= (B >> 5);
	A -= B; A -= C; A ^= (C >> 3);
	B -= C; B -= A; B ^= (A << 10);
	C -= A; C -= B; C ^= (B >> 15);

	return C;
}

inline size_t HashCombine2(size_t A, size_t C)
{
	size_t value = A;
	value ^= C + 0x9e3779b9 + (A << 6) + (A >> 2);
	return value;
}


struct Float3
{
	Float3() :
		x(0.0), y(0.0), z(0.0) {}
	Float3(float a) :
		x(a), y(a), z(a) {}
	Float3(float _x, float _y, float _z) :
		x(_x), y(_y), z(_z) {}

	float x;
	float y;
	float z;

	float& operator[](unsigned int i)
	{
		return (i == 0 ? x : (i == 1 ? y : z));
	}
	float& operator[](int i)
	{
		return (i == 0 ? x : (i == 1 ? y : z));
	}
	const float& operator[](unsigned int i) const
	{
		return (i == 0 ? x : (i == 1 ? y : z));
	}
	const float& operator[](int i) const
	{
		return (i == 0 ? x : (i == 1 ? y : z));
	}

	friend Float3 operator-(const Float3& a, const Float3& b);
	friend Float3 operator+(const Float3& a, const Float3& b);
	friend Float3 operator*(const Float3& a, const double b);
	friend Float3 operator*(const Float3& a, const Float3& b);
	friend Float3 operator/(const Float3& a, const double b);
	friend Float3 Normalize(const Float3& a);
	friend float Length(const Float3& a);
	friend Float3 Cross(const Float3& a, const Float3& b);
	friend float Dot(const Float3& a, const Float3& b);
};

Float3 CalculateNormal(Float3& x, Float3& y, Float3& z);

struct Float3_16
{
	Float3_16(float a = 0.0f) :
		x(a), y(a), z(a) {}
	Float3_16(numeric::float16_t a) :
		x(a), y(a), z(a) {}
	Float3_16(numeric::float16_t _x, numeric::float16_t _y, numeric::float16_t _z) :
		x(_x), y(_y), z(_z) {}
	Float3_16(Float3 a):
		x(a.x), y(a.y), z(a.z)
	{}

	numeric::float16_t x;
	numeric::float16_t y;
	numeric::float16_t z;

};


struct Uint3
{
	Uint3(uint a = 0) :
		x(a), y(a), z(a) {}
	Uint3(uint _x, uint _y, uint _z) :
		x(_x), y(_y), z(_z) {}
	Uint3(Float3 a) :
		x(a.x), y(a.y), z(a.z)
	{}

	uint x;
	uint y;
	uint z;
};

inline int BytesToUnsignedIntegerLittleEndian(Byte* Src, size_t Offset)
{
	return static_cast<int>(static_cast<Byte>(Src[Offset]) |
		static_cast<Byte>(Src[Offset + 1]) << 8 |
		static_cast<Byte>(Src[Offset + 2]) << 16 |
		static_cast<Byte>(Src[Offset + 3]) << 24);
}

inline float BytesToFloatLittleEndian(Byte* Src, size_t Offset)
{
	uint32_t value = static_cast<uint32_t>(Src[Offset]) |
		static_cast<uint32_t>(Src[Offset + 1]) << 8 |
		static_cast<uint32_t>(Src[Offset + 2]) << 16 |
		static_cast<uint32_t>(Src[Offset + 3]) << 24;

	return *reinterpret_cast<float*>(&value);
}

inline std::string BytesToASCIIString(Byte* Src, size_t Offset, int Length)
{
	char* Buffer = new char[size_t(Length) + 1];
	memcpy(Buffer, Src + Offset, Length);
	Buffer[Length] = '\0';

	std::string Str = Buffer;
	delete[] Buffer;
	return std::move(Str);
}


inline void WriteUnsignedIntegerToBytesLittleEndian(Byte* Src, size_t* Offset, uint Value)
{
	Src[*Offset] = Value & 0x000000ff;
	Src[*Offset + 1] = (Value & 0x0000ff00) >> 8;
	Src[*Offset + 2] = (Value & 0x00ff0000) >> 16;
	Src[*Offset + 3] = (Value & 0xff000000) >> 24;
	*Offset += 4;
}

inline void WriteUnsignedInteger16ToBytesLittleEndian(Byte* Src, size_t* Offset, std::uint16_t Value)
{
	Src[*Offset] = Value & 0x000000ff;
	Src[*Offset + 1] = (Value & 0x0000ff00) >> 8;
	*Offset += 2;
}


inline void WriteASCIIStringToBytes(Byte* Dst, size_t* Offset, std::string& Value)
{
	memcpy(Dst + *Offset, Value.c_str(), Value.length());
	*Offset += Value.length();
}

inline void WriteFloatToBytesLittleEndian(Byte* Src, size_t* Offset, float Value)
{
	uint* T = (uint*)&Value;
	uint V = *T;
	WriteUnsignedIntegerToBytesLittleEndian(Src, Offset, V);
}

inline void WriteFloat16ToBytesLittleEndian(Byte* Src, size_t* Offset, numeric::float16_t Value)
{
	std::uint16_t* T = (std::uint16_t*)&Value;
	std::uint16_t V = *T;
	WriteUnsignedInteger16ToBytesLittleEndian(Src, Offset, V);
}


static uint PackFloatsToUint(float a, float b)
{
	uint aScaled = a * 65535.0f;
	uint bScaled = b * 65535.0f;
	return (aScaled << 16) | (bScaled & 0xFFFF);
}

static void UnpackUintToFloats(uint v, float* a, float* b)
{
	uint uintInput = v;
	*a = (uintInput >> 16) / 65535.0f;
	*b = (uintInput & 0xFFFF) / 65535.0f;
}
/*
* Precision : 0.00005f
*/
static Uint3 EncodeNormalsToUint3(Float3 N1, Float3 N2)
{
	Uint3 Result = 0;
	Result.x = PackFloatsToUint(N1.x * 0.5f + 0.5f, N1.y * 0.5f + 0.5f);
	Result.y = PackFloatsToUint(N1.z * 0.5f + 0.5f, N2.x * 0.5f + 0.5f);
	Result.z = PackFloatsToUint(N2.y * 0.5f + 0.5f, N2.z * 0.5f + 0.5f);

	return Result;
}

static void DecodeUint3ToNormals(Uint3 V, Float3* N1, Float3* N2)
{
	Float3 R1 = 0.0f;
	Float3 R2 = 0.0f;
	UnpackUintToFloats(V.x, &R1.x, &R1.y);
	UnpackUintToFloats(V.y, &R1.z, &R2.x);
	UnpackUintToFloats(V.z, &R2.y, &R2.z);

	*N1 = R1 * 2.0f - 1.0f;
	*N2 = R2 * 2.0f - 1.0f;
}
