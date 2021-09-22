#include "Adjacency.h"
#include <iostream>
#include <filesystem>
#include <fstream>


#define WRITE_MESSAGE_DIGIT(Message, Digit) \
	MessageString += Message; \
	MessageString += std::to_string(Digit); \
	MessageString += "\n";

#define ELEMENT_LENGTH 4



inline int BytesToUnsignedIntegerLittleEndian(Byte* Src, int Offset)
{
	return static_cast<int>(static_cast<Byte>(Src[Offset]) |
		static_cast<Byte>(Src[Offset + 1]) << 8 |
		static_cast<Byte>(Src[Offset + 2]) << 16 |
		static_cast<Byte>(Src[Offset + 3]) << 24);
}

 
bool AdjacencyProcesser::GetReady(std::string& FilePath)
{
	if (AsyncProcesser == nullptr)
	{
		AsyncProcesser = new ThreadProcesser();
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

	TotalBytesNum = FileSize;
	WRITE_MESSAGE_DIGIT("Total Bytes Length: ", TotalBytesNum)
	
	BytesData = new Byte[FileSize];
	if (BytesData == nullptr)
	{
		ErrorString += "Create byte data failed.\n";
		return false;
	}

	InFile.read((char*)BytesData, FileSize);

	InFile.close();

	uint MeshNum = 0;
	uint BytesPerElement = 0;
	int Pos = 8;

	MeshNum = (uint)*(BytesData);
	// Since every index of triangle is 4 bytes length(Enough for modeling), this is temperally not using
	BytesPerElement = (uint)*(BytesData + 4);

	WRITE_MESSAGE_DIGIT("Total Mesh Num: ", MeshNum);
	WRITE_MESSAGE_DIGIT("Bytes Per Element: ", BytesPerElement);


	for (uint i = 0; i < MeshNum; i++)
	{
		int IndicesLength = BytesToUnsignedIntegerLittleEndian(BytesData, Pos);

		Pos += ELEMENT_LENGTH;
		Byte* Bytes = BytesData + Pos;
		Pos += ELEMENT_LENGTH * IndicesLength;

		SourceContext* Context = new SourceContext();
		Context->BytesData = Bytes;
		Context->TotalLength = ELEMENT_LENGTH * IndicesLength;
		Context->IndicesLength = IndicesLength;
		Context->TriangleNum = IndicesLength / 3;
		Context->CurrentFacePos = 0;
		Context->CurrentIndexPos = 0;
		Context->CurrentTrianglePos = 0;

		WRITE_MESSAGE_DIGIT("Mesh: ", i);
		WRITE_MESSAGE_DIGIT("Triangle: ", Context->TriangleNum);
		WRITE_MESSAGE_DIGIT("IndexLength: ", Context->IndicesLength);
		WRITE_MESSAGE_DIGIT("TotalLength: ", Context->TotalLength);

		AsyncProcesser->AddData((void*)Context);
		ContextList.push_back(Context);
	}


	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunFunc, this, std::placeholders::_1, std::placeholders::_2);
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

void* AdjacencyProcesser::RunFunc(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	uint x = BytesToUnsignedIntegerLittleEndian(Src->BytesData, Src->CurrentIndexPos); Src->CurrentIndexPos += ELEMENT_LENGTH;
	uint y = BytesToUnsignedIntegerLittleEndian(Src->BytesData, Src->CurrentIndexPos); Src->CurrentIndexPos += ELEMENT_LENGTH;
	uint z = BytesToUnsignedIntegerLittleEndian(Src->BytesData, Src->CurrentIndexPos); Src->CurrentIndexPos += ELEMENT_LENGTH;

	uint FaceId = Src->CurrentFacePos;

	Edge e1 = Edge(x, y);
	Edge e2 = Edge(y, z);
	Edge e3 = Edge(z, x);

	AdjFace adj = AdjFace();
	adj.face1 = FaceId;
	adj.set1 = true;

	std::unordered_map<Edge, AdjFace>::iterator it1;
	it1 = Src->EdgeList.find(e1);
	if (Src->EdgeList.count(e1) != 0)
	{
#if DEBUG_1
		if (it1->second.set2 == true)
		{
			ErrorString += "A edge has more than 2 face.\n";
			ErrorString += "Edge 1: " + std::to_string(e1.v1) + ", " + std::to_string(e1.v2) + " H " + std::to_string(e1.hash) + "\n";
			ErrorString += "Edge 2: " + std::to_string(it1->first.v1) + ", " + std::to_string(it1->first.v2) + " H " + std::to_string(it1->first.hash) + "\n";
		}
#endif
		it1->second.face2 = FaceId;
		it1->second.set2 = true;

#if DEBUG_2
		ErrorString += "Write Edge: Found " + std::to_string(e1.v1) + ", " + std::to_string(e1.v2) + " | " + std::to_string(it1->second.face1) + ", " + std::to_string(it1->second.face2) + "\n";
#endif
	}
	else
	{
		Src->EdgeList.insert(std::unordered_map<Edge, AdjFace>::value_type(e1, adj));
#if DEBUG_2
		ErrorString += "Write Edge: NOTFound " + std::to_string(e1.v1) + ", " + std::to_string(e1.v2) + " | " + std::to_string(adj.face1) + ", " + std::to_string(adj.face2) + "\n";
#endif
	}

	std::unordered_map<Edge, AdjFace>::iterator it2 = Src->EdgeList.find(e2);
	if (it2 != Src->EdgeList.end())
	{
#if DEBUG_1
		if (it2->second.set2 == true)
		{
			ErrorString += "A edge has more than 2 face.\n";
			ErrorString += "Edge 1: " + std::to_string(e2.v1) + ", " + std::to_string(e2.v2) + " H " + std::to_string(e2.hash) + "\n";
			ErrorString += "Edge 2: " + std::to_string(it2->first.v1) + ", " + std::to_string(it2->first.v2) + " H " + std::to_string(it2->first.hash) + "\n";
		}
#endif
		it2->second.face2 = FaceId;
		it2->second.set2 = true;
#if DEBUG_2
		ErrorString += "Write Edge: Found " + std::to_string(e2.v1) + ", " + std::to_string(e2.v2) + " | " + std::to_string(it2->second.face1) + ", " + std::to_string(it2->second.face2) + "\n";
#endif
	}
	else
	{
		Src->EdgeList.insert(std::unordered_map<Edge, AdjFace>::value_type(e2, adj));
#if DEBUG_2
		ErrorString += "Write Edge: NOTFound " + std::to_string(e2.v1) + ", " + std::to_string(e2.v2) + " | " + std::to_string(adj.face1) + ", " + std::to_string(adj.face2) + "\n";
#endif
	}

	std::unordered_map<Edge, AdjFace>::iterator it3 = Src->EdgeList.find(e3);
	if (it3 != Src->EdgeList.end())
	{
#if DEBUG_1
		if (it3->second.set2 == true)
		{
			ErrorString += "A edge has more than 2 face.\n";
			ErrorString += "Edge 1: " + std::to_string(e3.v1) + ", " + std::to_string(e3.v2) + " H " + std::to_string(e3.hash) + "\n";
			ErrorString += "Edge 2: " + std::to_string(it3->first.v1) + ", " + std::to_string(it3->first.v2) + " H " + std::to_string(it3->first.hash) + "\n";
		}
#endif
		it3->second.face2 = FaceId;
		it3->second.set2 = true;
#if DEBUG_2
		ErrorString += "Write Edge: Found " + std::to_string(e3.v1) + ", " + std::to_string(e3.v2) + " | " + std::to_string(it3->second.face1) + ", " + std::to_string(it3->second.face2) + "\n";
#endif
	}
	else
	{
		Src->EdgeList.insert(std::unordered_map<Edge, AdjFace>::value_type(e3, adj));
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