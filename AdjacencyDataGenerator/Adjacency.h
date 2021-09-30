#pragma once

#include "ThreadProcesser.h"
#include <unordered_map>
#include <fstream>
#include <filesystem>

typedef unsigned char Byte;
typedef unsigned int uint;


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


struct Vertex3
{
	Vertex3(float _x = 0, float _y = 0, float _z = 0) :
		x(_x), y(_y), z(_z)
	{
		hash = HashCombine2(std::hash<float>()(x), std::hash<float>()(y));
		hash = HashCombine2(hash, std::hash<float>()(z));
	}
	float x;
	float y;
	float z;
	size_t hash;
};


struct Face
{
	Face(uint _x, uint _y, uint _z):
		x(_x), y(_y), z(_z), set1(false) {}
	uint x;
	uint y;
	uint z;
	bool set1;

	uint GetOppositePoint(uint v1, uint  v2)
	{
		if (v1 == x)
		{
			if (v2 == y) return z;
			else if (v2 == z) return y;
		}
		else if (v1 == y)
		{
			if (v2 == x) return z;
			else if (v2 == z) return x;
		}
		else if(v1 == z)
		{
			if (v2 == x) return y;
			else if (v2 == y) return x;
		}

		return x;
	}
};

struct FacePair
{
	FacePair(uint _face1 = 0, uint _face2 = 0):
		face1(_face1), face2(_face2), set1(false), set2(false) {}

	uint face1;
	uint face2;
	bool set1;
	bool set2;
};



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
	Vertex3 ver1;
	Vertex3 ver2;
};

namespace std {
	template <> struct hash<Edge>
	{
		size_t operator()(const Edge& x) const
		{
			return x.hash;
		}
	};

	template <> struct hash<Vertex3>
	{
		size_t operator()(const Vertex3& x) const
		{
			return x.hash;
		}
	};
}

struct AdjFace
{
	AdjFace() :
		x(0), y(0), z(0), xy_adj(0), yz_adj(0), zx_adj(0),
		set1(false),
		set2(false),
		set3(false)
	{}
	AdjFace(const Face& face):
		x(face.x),
		y(face.y),
		z(face.z),
		xy_adj(0),
		yz_adj(0),
		zx_adj(0),
		set1(false),
		set2(false),
		set3(false)
	{}

	uint x;
	uint y;
	uint z;

	uint xy_adj;
	uint yz_adj;
	uint zx_adj;

	bool set1;
	bool set2;
	bool set3;
};

struct VertexContext
{
	VertexContext() :
		BytesData(nullptr),
		TotalLength(0),
		VertexLength(0),
		CurrentVertexPos(0),
		CurrentVertexId(0) {}

	Byte* BytesData;
	std::vector<Vertex3> RawVertex;
	std::unordered_map<Vertex3, uint> VertexList;
	std::unordered_map<uint, uint> IndexMap;
	uint TotalLength;
	uint VertexLength;

	uint CurrentVertexPos;
	uint CurrentVertexId;

	void DumpIndexMap()
	{
		std::ofstream OutFile("IndexMap.txt", std::ios::out);
		for (std::unordered_map<uint, uint>::iterator it = IndexMap.begin(); it != IndexMap.end(); it++)
		{
			OutFile << "Index : " << it->first << " -> " << it->second <<  std::endl;
		}
		OutFile.close();
	}

	void DumpVertexList()
	{
		std::ofstream OutFile("VertexList.txt", std::ios::out);
		for (std::unordered_map<Vertex3, uint>::iterator it = VertexList.begin(); it != VertexList.end(); it++)
		{
			OutFile << "Vertex : " << it->first.x << "," << it->first.y << "," << it->first.z << "|" << it->second << std::endl;
		}
		OutFile.close();
	}
};

struct SourceContext
{
	SourceContext():
		BytesData(nullptr), 
		TotalLength(0), 
		IndicesLength(0),
		TriangleNum(0),
		CurrentFacePos(0),
		CurrentIndexPos(0) {}

	Byte* BytesData;
	std::vector<Face> FaceList;
	std::unordered_map<Edge, FacePair> EdgeList;
	std::vector<AdjFace> AdjacencyFaceList;
	uint TotalLength;
	uint IndicesLength;
	uint TriangleNum;

	uint CurrentFacePos;
	uint CurrentIndexPos;

	void DumpFaceList()
	{
		std::ofstream OutFile("FaceList.txt", std::ios::out);
		for (std::vector<Face>::iterator it = FaceList.begin(); it != FaceList.end(); it++)
		{
			OutFile << "Face: " << it->x << ", " << it->y << ", " << it->z << std::endl;
		}
		OutFile.close();
	}

	void DumpEdgeList()
	{
		std::ofstream OutFile("EdgeList.txt", std::ios::out);
		for (std::unordered_map<Edge, FacePair>::iterator it = EdgeList.begin(); it != EdgeList.end(); it++)
		{
			OutFile << "Edge: " << it->first.v1 << ", " << it->first.v2 << "|" << "(" << it->second.face1 << "," << it->second.set1 << ") " << "(" << it->second.face2 << "," << it->second.set2 << ")" << std::endl;
		}
		OutFile.close();
	}

	void DumpAdjacencyFaceList()
	{
		std::ofstream OutFile("AdjList.txt", std::ios::out);
		for (std::vector<AdjFace>::iterator it = AdjacencyFaceList.begin(); it != AdjacencyFaceList.end(); it++)
		{
			OutFile << "AdjFace: " << it->x << ", " << it->y << ", " << it->z << " | " << it->xy_adj << "(" << it->set1 << ") " << it->yz_adj << "(" << it->set2 << ") " << it->zx_adj << "(" << it->set3 << ") " << std::endl;
		}
		OutFile.close();
	}
};

class AdjacencyProcesser
{
public:
	AdjacencyProcesser():
		AsyncProcesser(nullptr),
		ErrorString(""),
		MessageString(""),
		TriangleBytesData(nullptr),
		TotalTriangleBytesNum(0),
		VertexBytesData(nullptr),
		TotalVertexBytesNum(0)
	{}
	~AdjacencyProcesser()
	{
		if (AsyncProcesser != nullptr)
		{
			delete AsyncProcesser;
			AsyncProcesser = nullptr;
		}

		for (int i = 0; i < TriangleContextList.size(); i++)
		{
			delete TriangleContextList[i];
		}
		TriangleContextList.clear();

		for (int i = 0; i < VertexContextList.size(); i++)
		{
			delete VertexContextList[i];
		}
		VertexContextList.clear();

		if (TriangleBytesData != nullptr)
		{
			delete[] TriangleBytesData;
			TriangleBytesData = nullptr;
		}

		if (VertexBytesData != nullptr)
		{
			delete[] VertexBytesData;
			VertexBytesData = nullptr;
		}
	}

	//Pass 0
	bool GetReady0(std::filesystem::path& VertexFilePath);
	void* RunFunc0(void* SourceData, double* OutProgressPerRun);

	//Pass 1
	bool GetReady1(std::filesystem::path& FilePath);
	void* RunFunc1(void* SourceData, double* OutProgressPerRun);
	
	//Pass 2
	bool GetReady2();
	void* RunFunc2(void* SourceData, double* OutProgressPerRun);

	double GetProgress()
	{
		if (AsyncProcesser == nullptr)
			return 1.0;

		double Progress = 0.0;
		SourceContext* Result = (SourceContext*)AsyncProcesser->GetResult(&Progress);
		//if (Result) ContextList.push_back(Result);

		return Progress;
	}

	std::string& GetErrorString()
	{
		return ErrorString;
	}

	void DumpErrorString(uint FileIndex)
	{
		std::string FileName = "error" + std::to_string(FileIndex);
		FileName += ".log";

		std::ofstream OutFile(FileName.c_str(), std::ios::out);
		OutFile << ErrorString;
		OutFile.close();
		ErrorString = "";
	}

	std::string& GetMessageString()
	{
		return MessageString;
	}

	std::vector<SourceContext*>& GetTriangleContextList()
	{
		return TriangleContextList;
	}

	std::vector<VertexContext*>& GetVertexContextList()
	{
		return VertexContextList;
	}

	bool IsWorking()
	{
		return (AsyncProcesser != nullptr && AsyncProcesser->IsWorking());
	}

	void Clear()
	{
		if (IsWorking()) {
			AsyncProcesser->Stop();
			::Sleep(1.0);
		}

		if (TriangleContextList.size() > 0)
		{
			for (int i = 0; i < TriangleContextList.size(); i++)
			{
				delete TriangleContextList[i];
			}
			TriangleContextList.clear();
		}

		if (VertexContextList.size() > 0)
		{
			for (int i = 0; i < VertexContextList.size(); i++)
			{
				delete VertexContextList[i];
			}
			VertexContextList.clear();
		}

		ErrorString = "";
		MessageString = "";
	}

private:
	ThreadProcesser* AsyncProcesser;
	std::string ErrorString;
	std::string MessageString;

	Byte* TriangleBytesData;
	unsigned long long TotalTriangleBytesNum;

	Byte* VertexBytesData;
	unsigned long long TotalVertexBytesNum;

	std::vector<SourceContext*> TriangleContextList;
	std::vector<VertexContext*> VertexContextList;
};
