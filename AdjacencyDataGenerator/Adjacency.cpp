#include "Adjacency.h"
#include <iostream>


#define WRITE_MESSAGE_DIGIT(Message, Digit) \
	MessageString += Message; \
	MessageString += std::to_string(Digit); \
	MessageString += "\n";

#define ELEMENT_LENGTH 4
#define VER_ELEMENT_LENGTH 4


inline int BytesToUnsignedIntegerLittleEndian(Byte* Src, int Offset)
{
	return static_cast<int>(static_cast<Byte>(Src[Offset]) |
		static_cast<Byte>(Src[Offset + 1]) << 8 |
		static_cast<Byte>(Src[Offset + 2]) << 16 |
		static_cast<Byte>(Src[Offset + 3]) << 24);
}

inline float BytesToFloatLittleEndian(Byte* Src, int Offset)
{
	uint32_t value = static_cast<uint32_t>(Src[Offset]) |
		static_cast<uint32_t>(Src[Offset + 1]) << 8 |
		static_cast<uint32_t>(Src[Offset + 2]) << 16 |
		static_cast<uint32_t>(Src[Offset + 3]) << 24;

	return *reinterpret_cast<float*>(&value);
}
 



bool AdjacencyProcesser::GetReady0(std::filesystem::path& VertexFilePath)
{
	if (AsyncProcesser == nullptr)
	{
		AsyncProcesser = new ThreadProcesser();
	}
	else
	{
		Clear();
	}


	std::filesystem::path Path(VertexFilePath);
	if (!std::filesystem::exists(Path) || !std::filesystem::is_regular_file(Path))
	{
		ErrorString += "Cannot open the specified file.\n";
		return false;
	}
	std::ifstream InFile(VertexFilePath.c_str(), std::ios::in | std::ios::binary);

	uintmax_t FileSize = std::filesystem::file_size(Path);
	if (FileSize == static_cast<uintmax_t>(-1) || !InFile)
	{
		ErrorString += "The file is null.\n";
		return false;
	}

	size_t VertexTotalBytesNum = FileSize;
	WRITE_MESSAGE_DIGIT("Total Bytes Length: ", VertexTotalBytesNum)

	Byte* VertexBytesData = new Byte[VertexTotalBytesNum];
	if (VertexBytesData == nullptr)
	{
		ErrorString += "Create byte data failed.\n";
		return false;
	}

	InFile.read((char*)VertexBytesData, VertexTotalBytesNum);

	InFile.close();

	uint MeshNum = 0;
	uint BytesPerElement = 0;
	int Pos = 8;

	MeshNum = (uint) * (VertexBytesData);
	BytesPerElement = (uint) * (VertexBytesData + 4);

	WRITE_MESSAGE_DIGIT("Total Mesh Num: ", MeshNum);
	WRITE_MESSAGE_DIGIT("Bytes Per Element: ", BytesPerElement);


	for (uint i = 0; i < MeshNum; i++)
	{
		uint vertexLength = BytesToUnsignedIntegerLittleEndian(VertexBytesData, Pos);

		Pos += VER_ELEMENT_LENGTH;
		Byte* Bytes = VertexBytesData + Pos;
		Pos += VER_ELEMENT_LENGTH * 3 * vertexLength;

		VertexContext* Context = new VertexContext();
		Context->BytesData = Bytes;
		Context->TotalLength = VER_ELEMENT_LENGTH * 3 * vertexLength;
		Context->VertexLength = vertexLength;
		Context->CurrentVertexPos = 0;
		Context->CurrentVertexId = 0;


		WRITE_MESSAGE_DIGIT("Mesh: ", i);
		WRITE_MESSAGE_DIGIT("Vertex: ", Context->VertexLength);
		WRITE_MESSAGE_DIGIT("TotalLength: ", Context->TotalLength);

		AsyncProcesser->AddData((void*)Context);
		VertexContextList.push_back(Context);
	}

	
	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunFunc0, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}
	

	return true;
}


void* AdjacencyProcesser::RunFunc0(void* SourceData, double* OutProgressPerRun)
{
	VertexContext* Src = (VertexContext*)SourceData;

	uint x = BytesToFloatLittleEndian(Src->BytesData, Src->CurrentVertexPos); Src->CurrentVertexPos += VER_ELEMENT_LENGTH;
	uint y = BytesToFloatLittleEndian(Src->BytesData, Src->CurrentVertexPos); Src->CurrentVertexPos += VER_ELEMENT_LENGTH;
	uint z = BytesToFloatLittleEndian(Src->BytesData, Src->CurrentVertexPos); Src->CurrentVertexPos += VER_ELEMENT_LENGTH;

	Vertex3 v = Vertex3(x, y, z);
	uint VertexId = Src->CurrentVertexId;

	Src->RawVertex.push_back(v);

	std::unordered_map<Vertex3, uint>::iterator it = Src->VertexList.find(v);
	if (it == Src->VertexList.end())
	{
		Src->VertexList.insert(std::unordered_map<Vertex3, uint>::value_type(v, VertexId));
		Src->IndexMap.insert(std::unordered_map<uint, uint>::value_type(VertexId, VertexId));
	}
	else
	{
		Src->IndexMap.insert(std::unordered_map<uint, uint>::value_type(VertexId, it->second));
	}
	Src->CurrentVertexId++;

	double Step = 1.0 / Src->VertexLength;
	*OutProgressPerRun = Step;

	if (Src->CurrentVertexPos >= Src->TotalLength)
		return (void*)Src;
	else
		return nullptr;
}


bool AdjacencyProcesser::GetReady1(std::filesystem::path& FilePath)
{
	if (AsyncProcesser == nullptr)
	{
		AsyncProcesser = new ThreadProcesser();
	}
	else
	{
		Clear();
	}


	std::filesystem::path Path(FilePath);
	if(!std::filesystem::exists(Path) || !std::filesystem::is_regular_file(Path))
	{
		ErrorString += "Cannot open the specified file.\n";
		return false;
	}
	std::ifstream InFile(FilePath, std::ios::in | std::ios::binary);
	
	uintmax_t FileSize = std::filesystem::file_size(Path);
	if (FileSize == static_cast<uintmax_t>(-1) || !InFile)
	{
		ErrorString += "The file is null.\n";
		return false; 
	}

	TotalTriangleBytesNum = FileSize;
	WRITE_MESSAGE_DIGIT("Total Bytes Length: ", TotalTriangleBytesNum)
	
	TriangleBytesData = new Byte[FileSize];
	if (TriangleBytesData == nullptr)
	{
		ErrorString += "Create byte data failed.\n";
		return false;
	}

	InFile.read((char*)TriangleBytesData, FileSize);

	InFile.close();

	uint MeshNum = 0;
	uint BytesPerElement = 0;
	int Pos = 8;

	MeshNum = (uint)*(TriangleBytesData);
	// Since every index of triangle is 4 bytes length(Enough for modeling), this is temperally not using
	BytesPerElement = (uint)*(TriangleBytesData + 4);

	WRITE_MESSAGE_DIGIT("Total Mesh Num: ", MeshNum);
	WRITE_MESSAGE_DIGIT("Bytes Per Element: ", BytesPerElement);


	for (uint i = 0; i < MeshNum; i++)
	{
		int IndicesLength = BytesToUnsignedIntegerLittleEndian(TriangleBytesData, Pos);

		Pos += ELEMENT_LENGTH;
		Byte* Bytes = TriangleBytesData + Pos;
		Pos += ELEMENT_LENGTH * IndicesLength;

		SourceContext* Context = new SourceContext();
		Context->BytesData = Bytes;
		Context->TotalLength = ELEMENT_LENGTH * IndicesLength;
		Context->IndicesLength = IndicesLength;
		Context->TriangleNum = IndicesLength / 3;
		Context->CurrentFacePos = 0;
		Context->CurrentIndexPos = 0;

		WRITE_MESSAGE_DIGIT("Mesh: ", i);
		WRITE_MESSAGE_DIGIT("Triangle: ", Context->TriangleNum);
		WRITE_MESSAGE_DIGIT("IndexLength: ", Context->IndicesLength);
		WRITE_MESSAGE_DIGIT("TotalLength: ", Context->TotalLength);

		AsyncProcesser->AddData((void*)Context);
		TriangleContextList.push_back(Context);
	}


	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunFunc1, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

		
	return true;
}

#define DEBUG_1 1
#define DEBUG_2 0

void* AdjacencyProcesser::RunFunc1(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	uint x = BytesToUnsignedIntegerLittleEndian(Src->BytesData, Src->CurrentIndexPos); Src->CurrentIndexPos += ELEMENT_LENGTH;
	uint y = BytesToUnsignedIntegerLittleEndian(Src->BytesData, Src->CurrentIndexPos); Src->CurrentIndexPos += ELEMENT_LENGTH;
	uint z = BytesToUnsignedIntegerLittleEndian(Src->BytesData, Src->CurrentIndexPos); Src->CurrentIndexPos += ELEMENT_LENGTH;

	uint FaceId = Src->CurrentFacePos;

	Edge e1 = Edge(x, y);
	Edge e2 = Edge(y, z);
	Edge e3 = Edge(z, x);

	FacePair adj = FacePair();
	adj.face1 = FaceId;
	adj.set1 = true;

	std::unordered_map<Edge, FacePair>::iterator it1;
	it1 = Src->EdgeList.find(e1);
	if (Src->EdgeList.count(e1) != 0)
	{
#if DEBUG_1
		if (it1->second.set2 == true)
		{
			ErrorString += "A edge has more than 2 face. Face Id : " + std::to_string(FaceId) + "\n";
			ErrorString += "Edge 1: " + std::to_string(e1.v1) + ", " + std::to_string(e1.v2) + " H " + std::to_string(e1.hash) + "\n";
			ErrorString += "Edge 2: " + std::to_string(it1->first.v1) + ", " + std::to_string(it1->first.v2) + " H " + std::to_string(it1->first.hash) + "\n";
		}
		else
#endif
		{
			it1->second.face2 = FaceId;
			it1->second.set2 = true;
		}

#if DEBUG_2
		ErrorString += "Write Edge: Found " + std::to_string(e1.v1) + ", " + std::to_string(e1.v2) + " | " + std::to_string(it1->second.face1) + ", " + std::to_string(it1->second.face2) + "\n";
#endif
	}
	else
	{
		Src->EdgeList.insert(std::unordered_map<Edge, FacePair>::value_type(e1, adj));
#if DEBUG_2
		ErrorString += "Write Edge: NOTFound " + std::to_string(e1.v1) + ", " + std::to_string(e1.v2) + " | " + std::to_string(adj.face1) + ", " + std::to_string(adj.face2) + "\n";
#endif
	}

	std::unordered_map<Edge, FacePair>::iterator it2 = Src->EdgeList.find(e2);
	if (it2 != Src->EdgeList.end())
	{
#if DEBUG_1
		if (it2->second.set2 == true)
		{
			ErrorString += "A edge has more than 2 face. Face Id : " + std::to_string(FaceId) + "\n";
			ErrorString += "Edge 1: " + std::to_string(e2.v1) + ", " + std::to_string(e2.v2) + " H " + std::to_string(e2.hash) + "\n";
			ErrorString += "Edge 2: " + std::to_string(it2->first.v1) + ", " + std::to_string(it2->first.v2) + " H " + std::to_string(it2->first.hash) + "\n";
		}
		else
#endif
		{
			it2->second.face2 = FaceId;
			it2->second.set2 = true;
		}
#if DEBUG_2
		ErrorString += "Write Edge: Found " + std::to_string(e2.v1) + ", " + std::to_string(e2.v2) + " | " + std::to_string(it2->second.face1) + ", " + std::to_string(it2->second.face2) + "\n";
#endif
	}
	else
	{
		Src->EdgeList.insert(std::unordered_map<Edge, FacePair>::value_type(e2, adj));
#if DEBUG_2
		ErrorString += "Write Edge: NOTFound " + std::to_string(e2.v1) + ", " + std::to_string(e2.v2) + " | " + std::to_string(adj.face1) + ", " + std::to_string(adj.face2) + "\n";
#endif
	}

	std::unordered_map<Edge, FacePair>::iterator it3 = Src->EdgeList.find(e3);
	if (it3 != Src->EdgeList.end())
	{
#if DEBUG_1
		if (it3->second.set2 == true)
		{
			ErrorString += "A edge has more than 2 face. Face Id : " + std::to_string(FaceId) + "\n";
			ErrorString += "Edge 1: " + std::to_string(e3.v1) + ", " + std::to_string(e3.v2) + " H " + std::to_string(e3.hash) + "\n";
			ErrorString += "Edge 2: " + std::to_string(it3->first.v1) + ", " + std::to_string(it3->first.v2) + " H " + std::to_string(it3->first.hash) + "\n";
		}
		else
#endif
		{
			it3->second.face2 = FaceId;
			it3->second.set2 = true;
		}
#if DEBUG_2
		ErrorString += "Write Edge: Found " + std::to_string(e3.v1) + ", " + std::to_string(e3.v2) + " | " + std::to_string(it3->second.face1) + ", " + std::to_string(it3->second.face2) + "\n";
#endif
	}
	else
	{
		Src->EdgeList.insert(std::unordered_map<Edge, FacePair>::value_type(e3, adj));
#if DEBUG_2
		ErrorString += "Write Edge: NOTFound " + std::to_string(e3.v1) + ", " + std::to_string(e3.v2) + " | " + std::to_string(adj.face1) + ", " + std::to_string(adj.face2) + "\n";
#endif
	}

	Src->FaceList.push_back(Face(x, y, z));



	Src->CurrentFacePos++;
	
	double Step = 1.0 / Src->TriangleNum;
	*OutProgressPerRun = Step;

	if (Src->CurrentIndexPos >= Src->TotalLength)
		return (void*)Src;
	else
		return nullptr;
}



bool AdjacencyProcesser::GetReady2()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || TriangleBytesData == nullptr || IsWorking())
	{
		ErrorString += "GetReady1 has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (int i = 0; i < TriangleContextList.size(); i++) {
		WRITE_MESSAGE_DIGIT("Mesh: ", i);
		WRITE_MESSAGE_DIGIT("Face: ", TriangleContextList[i]->FaceList.size());
		WRITE_MESSAGE_DIGIT("Edge: ", TriangleContextList[i]->EdgeList.size());
	}


	for (uint i = 0; i < TriangleContextList.size() ; i++)
	{	
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;
		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}


	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunFunc2, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}


	return true;
}

#define DEBUG_3 1

void* AdjacencyProcesser::RunFunc2(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	uint FaceId = Src->CurrentFacePos;
	Face& CurrentFace = Src->FaceList[FaceId];

	if (CurrentFace.set1 == false) {
		AdjFace NewFace(CurrentFace);

		Edge e1 = Edge(CurrentFace.x, CurrentFace.y);
		Edge e2 = Edge(CurrentFace.y, CurrentFace.z);
		Edge e3 = Edge(CurrentFace.z, CurrentFace.x);

		std::unordered_map<Edge, FacePair>::iterator it1 = Src->EdgeList.find(e1);
		if (it1 != Src->EdgeList.end())
		{
			FacePair& adj = it1->second;
			if (adj.set1 && adj.face1 != FaceId)
			{
				NewFace.xy_adj = Src->FaceList[adj.face1].GetOppositePoint(e1.v1, e1.v2);
				NewFace.set1 = true;
				Src->FaceList[adj.face1].set1 = true;
			}
			else if(adj.set2 && adj.face2 != FaceId)
			{
				NewFace.xy_adj = Src->FaceList[adj.face2].GetOppositePoint(e1.v1, e1.v2);
				NewFace.set1 = true;
				Src->FaceList[adj.face2].set1 = true;
			}
		}
#if DEBUG_3
		else
		{
			ErrorString += "Find Edge Error. Current Face: " + std::to_string(FaceId) + " | " + std::to_string(CurrentFace.x) + "," + std::to_string(CurrentFace.y) + "," + std::to_string(CurrentFace.z) + "\n";
			ErrorString += "Edge: " + std::to_string(e1.v1) + ", " + std::to_string(e1.v2) + "\n";
		}
#endif

		std::unordered_map<Edge, FacePair>::iterator it2 = Src->EdgeList.find(e2);
		if (it2 != Src->EdgeList.end())
		{
			FacePair& adj = it2->second;
			if (adj.set1 && adj.face1 != FaceId)
			{
				NewFace.yz_adj = Src->FaceList[adj.face1].GetOppositePoint(e2.v1, e2.v2);
				NewFace.set2 = true;
				Src->FaceList[adj.face1].set1 = true;
			}
			else if (adj.set2 && adj.face2 != FaceId)
			{
				NewFace.yz_adj = Src->FaceList[adj.face2].GetOppositePoint(e2.v1, e2.v2);
				NewFace.set2 = true;
				Src->FaceList[adj.face2].set1 = true;
			}
		}
#if DEBUG_3
		else
		{
			ErrorString += "Find Edge Error. Current Face: " + std::to_string(FaceId) + " | " + std::to_string(CurrentFace.x) + "," + std::to_string(CurrentFace.y) + "," + std::to_string(CurrentFace.z) + "\n";
			ErrorString += "Edge: " + std::to_string(e2.v1) + ", " + std::to_string(e2.v2) + "\n";
		}
#endif

		std::unordered_map<Edge, FacePair>::iterator it3 = Src->EdgeList.find(e3);
		if (it3 != Src->EdgeList.end())
		{
			FacePair& adj = it3->second;
			if (adj.set1 && adj.face1 != FaceId)
			{
				NewFace.zx_adj = Src->FaceList[adj.face1].GetOppositePoint(e3.v1, e3.v2);
				NewFace.set3 = true;
				Src->FaceList[adj.face1].set1 = true;
			}
			else if (adj.set2 && adj.face2 != FaceId)
			{
				NewFace.zx_adj = Src->FaceList[adj.face2].GetOppositePoint(e3.v1, e3.v2);
				NewFace.set3 = true;
				Src->FaceList[adj.face2].set1 = true;
			}
		}
#if DEBUG_3
		else
		{
			ErrorString += "Find Edge Error. Current Face: " + std::to_string(FaceId) + " | " + std::to_string(CurrentFace.x) + "," + std::to_string(CurrentFace.y) + "," + std::to_string(CurrentFace.z) + "\n";
			ErrorString += "Edge: " + std::to_string(e3.v1) + ", " + std::to_string(e3.v2) + "\n";
		}
#endif

		Src->AdjacencyFaceList.push_back(NewFace);
		CurrentFace.set1 = true;
	}

	Src->CurrentFacePos++;

	double Step = 1.0 / Src->TriangleNum;
	*OutProgressPerRun = Step;

	if (Src->CurrentFacePos >= Src->TriangleNum)
		return (void*)Src;
	else
		return nullptr;
}