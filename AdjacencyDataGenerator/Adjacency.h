#pragma once

#include "ThreadProcesser.h"
#include <unordered_map>
#include <queue>
#include <set>
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

	bool operator==(const Vertex3& other) const
	{
		return (x == other.x && y == other.y && z == other.z);
	}

	bool operator!=(const Vertex3& other) const
	{
		return !(*this == other);
	}


	float x;
	float y;
	float z;
	size_t hash;
};

struct Index
{
	Index() :
		value(0), actual_value(0)
	{}
	Index(uint _value, uint _actual_value):
		value(_value), actual_value(_actual_value)
	{}

	bool operator<(const Index& other) const
	{
		return (value < other.value);
	}

	bool operator==(const Index& other) const
	{
		return (value == other.value);
	}

	bool operator!=(const Index& other) const
	{
		return !(*this == other);
	}

	Index operator=(const uint other)
	{
		value = other;
		actual_value = other;
		return *this;
	}

	uint value;
	uint actual_value;
};

struct Face
{
	explicit Face(Index _x, Index _y, Index _z):
		x(_x), y(_y), z(_z), IsRead(false), BlackWhite(0) {}
	Index x;
	Index y;
	Index z;

	bool IsRead;
	int BlackWhite;

	Index GetOppositePoint(Index v1, Index  v2)
	{
		if (v1.value == x.value)
		{
			if (v2.value == y.value) return z;
			else if (v2.value == z.value) return y;
		}
		else if (v1.value == y.value)
		{
			if (v2.value == x.value) return z;
			else if (v2.value == z.value) return x;
		}
		else if(v1.value == z.value)
		{
			if (v2.value == x.value) return y;
			else if (v2.value == y.value) return x;
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
	Edge(uint _v1, uint _v2, uint _actual_v1, uint _actual_v2) :
		v1(_v1, _actual_v1), v2(_v2, _actual_v2)
	{
		hash = HashCombine(std::min(v1.value, v2.value), std::max(v1.value, v2.value));
	}
	Edge(Index _v1, Index _v2) :
		v1(_v1), v2(_v2)
	{
		hash = HashCombine(std::min(v1.value, v2.value), std::max(v1.value, v2.value));
	}


	bool operator==(const Edge& other) const
	{
		return (v1.value == other.v1.value && v2.value == other.v2.value) || (v1.value == other.v2.value && v2.value == other.v1.value);
	}

	bool operator!=(const Edge& other) const
	{
		return !(*this == other);
	}

	size_t hash;
	Index v1;
	Index v2;
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
		x(0, 0), y(0, 0), z(0, 0)
	{
		adjPoint[0] = 0;
		adjPoint[1] = 0;
		adjPoint[2] = 0;
		adjFaceIndex[0] = 0;
		adjFaceIndex[1] = 0;
		adjFaceIndex[2] = 0;
		hasAdjFace[0] = false;
		hasAdjFace[1] = false;
		hasAdjFace[2] = false;
		faceIndex = 0;
	}
	AdjFace(const Face& face) :
		x(face.x),
		y(face.y),
		z(face.z)
	{
		adjPoint[0] = 0;
		adjPoint[1] = 0;
		adjPoint[2] = 0;
		adjFaceIndex[0] = 0;
		adjFaceIndex[1] = 0;
		adjFaceIndex[2] = 0;
		hasAdjFace[0] = false;
		hasAdjFace[1] = false;
		hasAdjFace[2] = false;
		faceIndex = 0;
	}

	Index x;
	Index y;
	Index z;

	// xy, yz, zx
	Index adjPoint[3]; 
	uint adjFaceIndex[3];
	bool hasAdjFace[3];

	uint faceIndex;
};

struct VertexContext
{
	VertexContext() :
		Name("None"),
		BytesData(nullptr),
		TotalLength(0),
		VertexLength(0),
		CurrentVertexPos(0),
		CurrentVertexId(0) {}

	std::string Name;
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

	void DumpRawVertexList()
	{
		std::ofstream OutFile("RawVertexList.txt", std::ios::out);
		for (std::vector<Vertex3>::iterator it = RawVertex.begin(); it != RawVertex.end(); it++)
		{
			OutFile << "Vertex : " << it->x << "," << it->y << "," << it->z  << std::endl;
		}
		OutFile.close();
	}
};

struct SourceContext
{
	SourceContext():
		Name("None"),
		BytesData(nullptr), 
		TotalLength(0), 
		IndicesLength(0),
		TriangleNum(0),
		CurrentFacePos(0),
		CurrentIndexPos(0),
		VertexData(nullptr)
	{}

	std::string Name;
	Byte* BytesData;
	//Pass1
	std::vector<Face> FaceList;
	std::unordered_map<Edge, FacePair> EdgeList;
	//Pass2
	std::queue<uint> FaceIdQueue;
	std::set<uint> FaceIdPool;
	std::vector<AdjFace> AdjacencyFaceList;
	//Pass3
	std::vector<AdjFace> AdjacencyFaceListShrink;

	uint TotalLength;
	uint IndicesLength;
	uint TriangleNum;

	uint CurrentFacePos;
	uint CurrentIndexPos;

	//Pass0
	VertexContext* VertexData;

	void DumpFaceList()
	{
		std::ofstream OutFile("FaceList.txt", std::ios::out);
		for (std::vector<Face>::iterator it = FaceList.begin(); it != FaceList.end(); it++)
		{
			OutFile << "Face: " << it->x.actual_value << ", " << it->y.actual_value << ", " << it->z.actual_value << std::endl;
		}
		OutFile.close();
	}

	void DumpEdgeList()
	{
		std::ofstream OutFile("EdgeList.txt", std::ios::out);
		for (std::unordered_map<Edge, FacePair>::iterator it = EdgeList.begin(); it != EdgeList.end(); it++)
		{
			OutFile << "Edge: " << it->first.v1.actual_value << ", " << it->first.v2.actual_value << "|" << "(" << it->second.face1 << "," << it->second.set1 << ") " << "(" << it->second.face2 << "," << it->second.set2 << ")" << std::endl;
		}
		OutFile.close();
	}

	void DumpAdjacencyFaceList()
	{
		std::ofstream OutFile("AdjList.txt", std::ios::out);
		for (std::vector<AdjFace>::iterator it = AdjacencyFaceList.begin(); it != AdjacencyFaceList.end(); it++)
		{
			OutFile << "AdjFace: " << it->x.actual_value << ", " << it->y.actual_value << ", " << it->z.actual_value << " | " 
				<< it->adjPoint[0].actual_value << "(" << it->hasAdjFace[0] << ") " << it->adjPoint[1].actual_value << "(" << it->hasAdjFace[1] << ") " << it->adjPoint[2].actual_value << "(" << it->hasAdjFace[2] << ") " 
				<< " | " << it->adjFaceIndex[0] << ", " << it->adjFaceIndex[1] << ", "  << it->adjFaceIndex[2]<< std::endl;
		}
		OutFile.close();
	}

	void DumpAdjacencyFaceListShrink()
	{
		std::ofstream OutFile("AdjList.txt", std::ios::out);
		for (std::vector<AdjFace>::iterator it = AdjacencyFaceListShrink.begin(); it != AdjacencyFaceListShrink.end(); it++)
		{
			OutFile << "AdjFace: " << it->x.actual_value << ", " << it->y.actual_value << ", " << it->z.actual_value << " | "
				<< it->adjPoint[0].actual_value << "(" << it->hasAdjFace[0] << ") " << it->adjPoint[1].actual_value << "(" << it->hasAdjFace[1] << ") " << it->adjPoint[2].actual_value << "(" << it->hasAdjFace[2] << ") "
				<< " | " << it->adjFaceIndex[0] << ", " << it->adjFaceIndex[1] << ", " << it->adjFaceIndex[2] << std::endl;
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
	bool GetReady1(std::filesystem::path& FilePath, bool NeedMergeDuplicateVertex = false);
	void* RunFunc1(void* SourceData, double* OutProgressPerRun);
	
	//Pass 2
	bool GetReady2();
	void* RunFunc2(void* SourceData, double* OutProgressPerRun);

	//Pass 3
	bool GetReady3();
	void* RunFunc3(void* SourceData, double* OutProgressPerRun);

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

	void ClearMessageString()
	{
		MessageString = "";
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
		AsyncProcesser->Clear();

		ErrorString = "";
		MessageString = "";
	}

	void ExportAdjacencyList(std::filesystem::path& FilePath);

private:
	bool HandleAdjacencyFace(
		const uint CurrentFaceId,
		const Edge& EdgeToSearch,
		uint EdgeIndex,
		SourceContext* Src,
		uint* OutAdjFaceId, AdjFace* OutAdjFace);
	bool QueryAdjacencyFace(
		const uint CurrentFaceId,
		const Edge& EdgeToSearch,
		uint EdgeIndex,
		SourceContext* Src,
		uint* OutAdjFaceId);
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
