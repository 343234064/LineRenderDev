#pragma once

#include "ThreadProcesser.h"
#include <unordered_map>

typedef unsigned char Byte;
typedef unsigned int uint;

struct Face
{
	Face(uint _x, uint _y, uint _z):
		x(_x), y(_y), z(_z) {}
	uint x;
	uint y;
	uint z;
};

struct AdjFace
{
	AdjFace(uint _face1 = 0, uint _face2 = 0):
		face1(_face1), face2(_face2), set1(false), set2(false) {}

	uint face1;
	uint face2;
	bool set1;
	bool set2;
};

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


class Edge
{
public:
	Edge(size_t _hash, uint _v1 = 0, uint _v2 = 0):
		hash(_hash), v1(_v1), v2(_v2) 
	{}

	Edge(uint _v1 = 0, uint _v2 = 0) :
		v1(_v1), v2(_v2)
	{
		hash = HashCombine(std::min(v1, v2), std::max(v1, v2));
	}

	bool operator==(const Edge& other) const
	{
		return (v1 == other.v1 && v2 == other.v2) || (v1 == other.v2 && v2 == other.v1);
	}

	bool operator!=(const Edge& other) const
	{
		return !(*this == other);
	}

	size_t hash;
	uint v1;
	uint v2;

};

namespace std {
	template <> struct hash<Edge>
	{
		size_t operator()(const Edge& x) const
		{
			return x.hash;
		}
	};
}



struct SourceContext
{
	Byte* BytesData;
	std::vector<Face> FaceList;
	std::unordered_map<Edge, AdjFace> EdgeList;
	uint TotalLength;
	uint IndicesLength;
	uint TriangleNum;

	uint CurrentFacePos;
	uint CurrentTrianglePos;
	uint CurrentIndexPos;
};

class AdjacencyProcesser
{
public:
	AdjacencyProcesser():
		AsyncProcesser(nullptr),
		ErrorString(""),
		MessageString(""),
		BytesData(nullptr),
		TotalBytesNum(0)

	{}
	~AdjacencyProcesser()
	{
		if (AsyncProcesser != nullptr)
		{
			delete AsyncProcesser;
			AsyncProcesser = nullptr;
		}

		for (int i = 0; i < ContextList.size(); i++)
		{
			delete ContextList[i];
		}
		ContextList.clear();

		if (BytesData != nullptr)
		{
			delete[] BytesData;
			BytesData = nullptr;
		}
	}

	bool GetReady(std::string& FilePath);
	
	void* RunFunc(void* SourceData, double* OutProgressPerRun);

	double GetProgress()
	{
		if (AsyncProcesser == nullptr)
			return 1.0;

		double Progress = 0.0;
		SourceContext* Result = (SourceContext*)AsyncProcesser->GetResult(&Progress);
		//if (Result) ContextList.push_back(Result);

		return Progress;
	}

	const char* GetErrorString()
	{
		return ErrorString.c_str();
	}

	const char* GetMessageString()
	{
		return MessageString.c_str();
	}

	std::vector<SourceContext*>& GetContextList()
	{
		return ContextList;
	}

	bool IsWorking()
	{
		return (AsyncProcesser != nullptr && AsyncProcesser->IsWorking());
	}

private:
	ThreadProcesser* AsyncProcesser;
	std::string ErrorString;
	std::string MessageString;

	Byte* BytesData;
	unsigned long long TotalBytesNum;

	std::vector<SourceContext*> ContextList;
};
