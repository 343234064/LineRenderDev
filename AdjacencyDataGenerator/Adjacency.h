#pragma once

#include "ThreadProcesser.h"
#include <unordered_map>
#include <queue>
#include <set>
#include <fstream>
#include <filesystem>

typedef unsigned char Byte;
typedef unsigned int uint;

using namespace std;

#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))

#define LINE_STRING "================================"
class AdjacencyProcesser;
bool PassGenerateVertexMap(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State);
bool PassGenerateFaceAndEdgeData(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State);
bool PassGenerateAdjacencyData(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State);
bool PassShrinkAdjacencyData(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State);
bool PassGenerateAdjacencyVertexMap(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State);
bool PassSerializeAdjacencyVertexMap(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State);
bool PassGenerateRenderData(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State);
void Export(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State);


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
	Float3(float _x, float _y, float _z):
		x(_x), y(_y), z(_z) {}

	float x;
	float y;
	float z;

	friend Float3 operator-(const Float3& a, const Float3& b);
	friend Float3 operator+(const Float3& a, const Float3& b);
	friend Float3 operator*(const Float3& a, const double b);
	friend Float3 operator*(const Float3& a, const Float3& b);
	friend Float3 Normalize(const Float3& a);
	friend float Length(const Float3& a);
	friend Float3 Cross(const Float3& a, const Float3& b);
	friend float Dot(const Float3& a, const Float3& b);
};



struct BoundingBox
{
	BoundingBox() :
		Center(0.0),
		HalfLength(1.0),
		Min(-1.0),
		Max(1.0)
	{}

	void Resize(Float3& Point)
	{
		Min.x = MIN(Point.x, Min.x);
		Min.y = MIN(Point.y, Min.y);
		Min.z = MIN(Point.z, Min.z);

		Max.x = MAX(Point.x, Max.x);
		Max.y = MAX(Point.y, Max.y);
		Max.z = MAX(Point.z, Max.z);

		HalfLength = (Max - Min) * 0.5;
		Center = Min + HalfLength;
	}

	void Resize(BoundingBox& Box)
	{
		Min.x = MIN(Box.Min.x, Min.x);
		Min.y = MIN(Box.Min.y, Min.y);
		Min.z = MIN(Box.Min.z, Min.z);

		Max.x = MAX(Box.Max.x, Max.x);
		Max.y = MAX(Box.Max.y, Max.y);
		Max.z = MAX(Box.Max.z, Max.z);

		HalfLength = (Max - Min) * 0.5;
		Center = Min + HalfLength;
	}

	void Clear()
	{
		Center = Float3(0.0);
		HalfLength = Float3(1.0);
		Min = Float3(-1.0);
		Max = Float3(1.0);
	}

	Float3 Center;
	Float3 HalfLength;
	Float3 Min;
	Float3 Max;
};



struct DrawRawVertex
{
	DrawRawVertex():
		pos(), color()
	{}
	DrawRawVertex(Float3 _pos, Float3 _nor, Float3 _col):
		pos(_pos), normal(_nor), color(_col)
	{}
	Float3 pos;
	Float3 normal;
	Float3 color;
};
typedef unsigned int DrawRawIndex;


#define EPS 0.000001f
struct Vertex3
{
	Vertex3(float _x = 0, float _y = 0, float _z = 0) :
		x(_x), y(_y), z(_z)
	{
		hash = HashCombine2(std::hash<float>()(x), std::hash<float>()(y));
		hash = HashCombine2(hash, std::hash<float>()(z));
	}
	Vertex3(const Vertex3& v):
		x(v.x), y(v.y), z(v.z), hash(v.hash)
	{}

	bool operator==(const Vertex3& other) const
	{
		return ((abs(x - other.x) < EPS) 
			&& (abs(y - other.y) < EPS)
			&& (abs(z - other.z) < EPS));
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

class Index
{
public:
	Index():
		value(0), actual_value(0)
	{
		hash = std::hash<uint>()(0);
	}
	Index(uint _value, uint _actual_value):
		value(_value), actual_value(_actual_value)
	{
		hash = std::hash<uint>()(_value);
	}
	Index(const Index& _index):
		value(_index.value), actual_value(_index.actual_value), hash(_index.hash)
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
		hash = std::hash<uint>()(other);

		return *this;
	}

	size_t hash;
	uint value;
	uint actual_value;
};

class Face
{
public:
	explicit Face(Index _x, Index _y, Index _z, uint e1, uint e2, uint e3):
		x(_x), y(_y), z(_z), 
		xy(e1), yz(e2), zx(e3),
		IsRead(false), IsAddToAdjFaceList(false), BlackWhite(0) {}
	Index x;
	Index y;
	Index z;
	//edge index
	uint xy;
	uint yz;
	uint zx;

	bool IsRead;
	bool IsAddToAdjFaceList;
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
		v1(_v1, _actual_v1), v2(_v2, _actual_v2), id(0)
	{
		hash = HashCombine(MIN(v1.value, v2.value), MAX(v1.value, v2.value));
	}
	Edge(Index _v1, Index _v2) :
		v1(_v1), v2(_v2), id(0)
	{
		hash = HashCombine(MIN(v1.value, v2.value), MAX(v1.value, v2.value));
	}
	Edge(const Edge& e):
		v1(e.v1), v2(e.v2), hash(e.hash), id(e.id)
	{}

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

	uint id;
};

namespace std {
	template <> struct hash<Index>
	{
		size_t operator()(const Index& x) const
		{
			return x.hash;
		}
	};

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

class AdjFace
{
public:
	AdjFace() = delete;
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
public:
	static uint ByteSize()
	{
		return sizeof(uint) * 6;
	}
};

class AdjVertex
{
public:
	AdjVertex() = delete;
	AdjVertex(const Vertex3& _v) :
		x(_v.x), y(_v.y), z(_v.z), adjId(0), adjSerializedId(0), adjNum(0)
	{}

	float x;
	float y;
	float z;
	uint adjId;
	uint adjSerializedId;
	uint adjNum;

public:
	static uint ByteSize()
	{
		return 3 * sizeof(float) + 2 * sizeof(uint);
	}
};


class VertexContext
{
public:
	VertexContext() :
		Name("None"),
		BytesData(nullptr),
		DrawVertexList(nullptr),
		TotalLength(0),
		VertexLength(0),
		CurrentVertexPos(0),
		CurrentVertexId(0) {}
	~VertexContext()
	{
		if (DrawVertexList != nullptr)
			delete[] DrawVertexList;
		DrawVertexList = nullptr;
	}

public:
	std::string Name;
	Byte* BytesData;
	std::vector<Vertex3> RawVertex;
	// shrinked vertex list without repeated vertex
	std::vector<AdjVertex> VertexList;
	// vertex data -> merged vertex  index
	std::unordered_map<Vertex3, uint> VertexMap;
	// actual vertex index in BytesData -> merged vertex  index 
	std::unordered_map<uint, uint> IndexMap;

	// for debug rendering
	DrawRawVertex* DrawVertexList;
	BoundingBox Bounding;

	uint TotalLength;
	uint VertexLength;

	uint CurrentVertexPos;
	uint CurrentVertexId;

public:
	void DumpIndexMap()
	{
		std::ofstream OutFile(Name + "_IndexMap.txt", std::ios::out);
		for (std::unordered_map<uint, uint>::iterator it = IndexMap.begin(); it != IndexMap.end(); it++)
		{
			OutFile << "Index : " << " actual: " << it->first  << " -> " << "virtual:" << it->second << std::endl;
		}
		OutFile.close();
	}

	void DumpVertexList()
	{
		std::ofstream OutFile(Name + "_VertexList.txt", std::ios::out);
		uint i = 0;
		for (std::vector<AdjVertex>::iterator it = VertexList.begin(); it != VertexList.end(); it++, i++)
		{
			OutFile << "Vertex : id " << i << " | " << it->x << "," << it->y << "," << it->z << " | adjId: " << it->adjId << " adjNum: " << it->adjNum << std::endl;
		}
		OutFile.close();
	}

	void DumpVertexMap()
	{
		std::ofstream OutFile(Name + "_VertexMap.txt", std::ios::out);
		uint i = 0;
		for (std::unordered_map<Vertex3, uint>::iterator it = VertexMap.begin(); it != VertexMap.end(); it++, i++)
		{
			OutFile << "Vertex : id " << i << " | " << it->first.x << "," << it->first.y << "," << it->first.z << " | virtual: " << it->second << std::endl;
		}
		OutFile.close();
	}

	void DumpRawVertexList()
	{
		std::ofstream OutFile(Name + "_RawVertexList.txt", std::ios::out);
		for (std::vector<Vertex3>::iterator it = RawVertex.begin(); it != RawVertex.end(); it++)
		{
			OutFile << "Vertex : " << it->x << "," << it->y << "," << it->z  << std::endl;
		}
		OutFile.close();
	}

};

class SourceContext
{
public:
	SourceContext():
		Name("None"),
		BytesData(nullptr), 
		DrawIndexList(nullptr),
		DrawIndexLineList(nullptr),
		TotalLength(0), 
		IndicesLength(0),
		TriangleNum(0),
		TotalAdjacencyVertexNum(0),
		CurrentFacePos(0),
		CurrentIndexPos(0),
		VertexData(nullptr)
	{}
	~SourceContext()
	{
		if (DrawIndexList != nullptr)
			delete[] DrawIndexList;
		DrawIndexList = nullptr;
		if (DrawIndexLineList != nullptr)
			delete[] DrawIndexLineList;
		DrawIndexLineList = nullptr;
	}

public:
	std::string Name;
	Byte* BytesData;
	//Pass0
	VertexContext* VertexData;
	//Pass1
	std::vector<Face> FaceList;
	std::vector<Edge> EdgeList;
	std::unordered_map<Edge, FacePair> EdgeFaceList;
	//Pass2
	std::queue<uint> FaceIdQueue;
	std::set<uint> FaceIdPool;
	std::vector<AdjFace> AdjacencyFaceList;
	//Pass3
	std::vector<AdjFace> AdjacencyFaceListShrink;
	//Pass4
	std::unordered_map<uint, std::set<uint>> AdjacencyVertexMap; // vertex -> adjacency vertex
	//Pass5
	std::vector<std::vector<uint>> AdjacencyVertexList;
	
	// for debug rendering
	DrawRawIndex* DrawIndexList;
	DrawRawIndex* DrawIndexLineList;

	uint TotalLength;
	uint IndicesLength;
	uint TriangleNum;
	uint TotalAdjacencyVertexNum;

	uint CurrentFacePos;
	uint CurrentIndexPos;

public:
	uint GetVertexDataByteSize()
	{
		return (uint)(VertexData->VertexList.size() * AdjVertex::ByteSize());
	}
	uint GetAdjacencyVertexListByteSize()
	{
		return (uint)(TotalAdjacencyVertexNum * sizeof(uint));
	}
	uint GetAdjacencyFaceListByteSize()
	{
		return (uint)(AdjacencyFaceListShrink.size() * AdjFace::ByteSize());
	}

	void DumpFaceList()
	{
		std::ofstream OutFile(Name + "_FaceList.txt", std::ios::out);
		uint i = 0;
		for (std::vector<Face>::iterator it = FaceList.begin(); it != FaceList.end(); it++, i++)
		{
			OutFile << "Face: id " << i << " | " << it->x.value << ", " << it->y.value << ", " << it->z.value << "|" << it->xy << ", " << it->yz << ", " << it->zx << std::endl;
		}
		OutFile.close();
	}

	void DumpEdgeList()
	{
		std::ofstream OutFile(Name + "_EdgeList.txt", std::ios::out);
		for (std::vector<Edge>::iterator it = EdgeList.begin(); it != EdgeList.end(); it++)
		{
			OutFile << "Edge: id " << it->id << " | " << it->v1.value << ", " << it->v2.value << std::endl;
		}
		OutFile.close();
	}

	void DumpEdgeFaceList()
	{
		std::ofstream OutFile(Name + "_EdgeFaceList.txt", std::ios::out);
		for (std::unordered_map<Edge, FacePair>::iterator it = EdgeFaceList.begin(); it != EdgeFaceList.end(); it++)
		{
			OutFile << "EdgeFace: " << it->first.v1.value << ", " << it->first.v2.value << "|" << "(" << it->second.face1 << "," << it->second.set1 << ") " << "(" << it->second.face2 << "," << it->second.set2 << ")" << std::endl;
		}
		OutFile.close();
	}

	void DumpAdjacencyFaceList()
	{
		std::ofstream OutFile(Name + "_AdjList.txt", std::ios::out);
		for (std::vector<AdjFace>::iterator it = AdjacencyFaceList.begin(); it != AdjacencyFaceList.end(); it++)
		{
			OutFile << "AdjFace: " << it->x.value << ", " << it->y.value << ", " << it->z.value << " | "
				<< it->adjPoint[0].value << "(" << it->hasAdjFace[0] << ") " << it->adjPoint[1].value << "(" << it->hasAdjFace[1] << ") " << it->adjPoint[2].value << "(" << it->hasAdjFace[2] << ") "
				<< " | " << it->adjFaceIndex[0] << ", " << it->adjFaceIndex[1] << ", "  << it->adjFaceIndex[2]<< std::endl;
		}
		OutFile.close();
	}

	void DumpAdjacencyFaceListShrink()
	{
		std::ofstream OutFile(Name + "_AdjList.txt", std::ios::out);
		for (std::vector<AdjFace>::iterator it = AdjacencyFaceListShrink.begin(); it != AdjacencyFaceListShrink.end(); it++)
		{
			OutFile << "AdjFace: " << it->x.value << ", " << it->y.value << ", " << it->z.value << " | "
				<< it->adjPoint[0].value << "(" << it->hasAdjFace[0] << ") " << it->adjPoint[1].value << "(" << it->hasAdjFace[1] << ") " << it->adjPoint[2].value << "(" << it->hasAdjFace[2] << ") "
				<< " | " << it->adjFaceIndex[0] << ", " << it->adjFaceIndex[1] << ", " << it->adjFaceIndex[2] << std::endl;
		}
		OutFile.close();
	}

	void DumpAdjacencyVertexList()
	{
		std::ofstream OutFile(Name + "_AdjVertex.txt", std::ios::out);
		for (std::vector<AdjVertex>::iterator it = VertexData->VertexList.begin(); it != VertexData->VertexList.end(); it++)
		{
			OutFile << "AdjVertex: " << it->x << ", " << it->y << ", " << it->z << " | "
				<< " adjId: " << it->adjId << " adjSerializedId: " << it->adjSerializedId << " | " << " adjNum: " << it->adjNum << " | ";
			for (std::vector<uint>::iterator it2 = AdjacencyVertexList[it->adjId].begin(); it2 != AdjacencyVertexList[it->adjId].end(); it2++)
			{
				OutFile << *it2 << " ";
			}
			OutFile << std::endl;
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

	//Pass 3
	bool GetReady3();
	void* RunFunc3(void* SourceData, double* OutProgressPerRun);

	//Pass 4
	bool GetReady4();
	void* RunFunc4(void* SourceData, double* OutProgressPerRun);

	//Pass 5
	bool GetReady5();
	void* RunFunc5(void* SourceData, double* OutProgressPerRun);

	//Pass Generate Render Data
	bool GetReadyGenerateRenderData();
	void* RunFuncGenerateRenderData(void* SourceData, double* OutProgressPerRun);

	double GetProgress()
	{
		if (AsyncProcesser == nullptr)
			return 0.0;

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


	void Export(std::filesystem::path& FilePath);
	
	

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

	void ExportAdjacencyFaceList(Byte* Buffer, uint* BytesOffset, const SourceContext* Context);
	void ExportAdjacencyVertexList(Byte* Buffer, uint* BytesOffset, const SourceContext* Context);
	void ExportVertexData(Byte* Buffer, uint* BytesOffset, const SourceContext* Context);

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
