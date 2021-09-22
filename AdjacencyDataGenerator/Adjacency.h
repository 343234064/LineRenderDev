#pragma once

#include "ThreadProcesser.h"
#include <map>

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

struct Edge
{
	Edge(uint _v1 = 0, uint _v2 = 0):
		v1(_v1), v2(_v2) {}

	bool operator==(const Edge& other)
	{
		return (v1 == other.v1 && v2 == other.v2) || (v1 == other.v2 && v2 == other.v1);
	}

	bool operator!=(const Edge& other)
	{
		return !(*this == other);
	}

	friend bool operator<(const Edge& other1, const Edge& other2)
	{
		return other1.v1 < other2.v1;
	}

	uint v1;
	uint v2;
};

struct SourceContext
{
	Byte* BytesData;
	std::vector<Face> FaceList;
	std::map<Edge, AdjFace> EdgeList;
	uint TotalLength;
	uint IndicesLength;
	uint TriangleNum;
	uint CurrentFacePos;
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
