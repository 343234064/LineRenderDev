#pragma once

#include <iostream>

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





inline int BytesToUnsignedIntegerLittleEndian(Byte* Src, uint Offset)
{
	return static_cast<int>(static_cast<Byte>(Src[Offset]) |
		static_cast<Byte>(Src[Offset + 1]) << 8 |
		static_cast<Byte>(Src[Offset + 2]) << 16 |
		static_cast<Byte>(Src[Offset + 3]) << 24);
}

inline float BytesToFloatLittleEndian(Byte* Src, uint Offset)
{
	uint32_t value = static_cast<uint32_t>(Src[Offset]) |
		static_cast<uint32_t>(Src[Offset + 1]) << 8 |
		static_cast<uint32_t>(Src[Offset + 2]) << 16 |
		static_cast<uint32_t>(Src[Offset + 3]) << 24;

	return *reinterpret_cast<float*>(&value);
}

inline std::string BytesToASCIIString(Byte* Src, int Offset, int Length)
{
	char* Buffer = new char[size_t(Length) + 1];
	memcpy(Buffer, Src + Offset, Length);
	Buffer[Length] = '\0';

	std::string Str = Buffer;
	delete[] Buffer;
	return std::move(Str);
}


inline void WriteUnsignedIntegerToBytesLittleEndian(Byte* Src, uint* Offset, uint Value)
{
	Src[*Offset] = Value & 0x000000ff;
	Src[*Offset + 1] = (Value & 0x0000ff00) >> 8;
	Src[*Offset + 2] = (Value & 0x00ff0000) >> 16;
	Src[*Offset + 3] = (Value & 0xff000000) >> 24;
	*Offset += 4;
}

inline void WriteASCIIStringToBytes(Byte* Dst, uint* Offset, std::string& Value)
{
	memcpy(Dst + *Offset, Value.c_str(), Value.length());
	*Offset += Value.length();
}

inline void WriteFloatToBytesLittleEndian(Byte* Src, uint* Offset, float Value)
{
	uint* T = (uint*)&Value;
	uint V = *T;
	WriteUnsignedIntegerToBytesLittleEndian(Src, Offset, V);
}
