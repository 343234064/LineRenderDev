#include "Adjacency.h"
#include <iostream>

#include "Utils.h"

#define WRITE_MESSAGE(Message, Obj) \
	MessageString += Message; \
	MessageString += Obj; \
	MessageString += "\n";

#define WRITE_MESSAGE_DIGIT(Message, Digit) \
	MessageString += Message; \
	MessageString += std::to_string(Digit); \
	MessageString += "\n";

#define ELEMENT_LENGTH 4
#define VER_ELEMENT_LENGTH 4




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

	if (VertexBytesData) delete[] VertexBytesData;
	VertexBytesData = new Byte[VertexTotalBytesNum];
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
		VertexContext* Context = new VertexContext();

		uint NameLength = BytesToUnsignedIntegerLittleEndian(VertexBytesData, Pos);
		Pos += 4;
		Context->Name = BytesToASCIIString(VertexBytesData, Pos, NameLength);
		Pos += NameLength;

		uint VertexLength = BytesToUnsignedIntegerLittleEndian(VertexBytesData, Pos);
		Pos += 4;

		Byte* Bytes = VertexBytesData + Pos;
		Pos += VER_ELEMENT_LENGTH * 3 * VertexLength;

		Context->BytesData = Bytes;
		Context->TotalLength = VER_ELEMENT_LENGTH * 3 * VertexLength;
		Context->VertexLength = VertexLength;
		Context->CurrentVertexPos = 0;
		Context->CurrentVertexId = 0;
		Context->RawVertex.reserve(VertexLength);

		WRITE_MESSAGE_DIGIT("Mesh: ", i);
		WRITE_MESSAGE("Name: ", Context->Name);
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

	float x = BytesToFloatLittleEndian(Src->BytesData, Src->CurrentVertexPos); Src->CurrentVertexPos += VER_ELEMENT_LENGTH;
	float y = BytesToFloatLittleEndian(Src->BytesData, Src->CurrentVertexPos); Src->CurrentVertexPos += VER_ELEMENT_LENGTH;
	float z = BytesToFloatLittleEndian(Src->BytesData, Src->CurrentVertexPos); Src->CurrentVertexPos += VER_ELEMENT_LENGTH;

	Vertex3 v = Vertex3(x, y, z);
	uint ActualVertexId = Src->CurrentVertexId;

	Src->RawVertex.emplace_back(v);

	// VertexMap : vertex data -> virtual vertex  index
	std::unordered_map<Vertex3, uint>::iterator it = Src->VertexMap.find(v);
	if (it == Src->VertexMap.end())
	{
		uint MergedVertexId = Src->VertexList.size();
		Src->VertexMap.insert(std::unordered_map<Vertex3, uint>::value_type(v, MergedVertexId));
		Src->IndexMap.insert(std::unordered_map<uint, uint>::value_type(ActualVertexId, MergedVertexId));
		Src->VertexList.push_back(v);
	}
	else
	{
		Src->IndexMap.insert(std::unordered_map<uint, uint>::value_type(ActualVertexId, it->second));
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
		AsyncProcesser->Clear();
		ErrorString = "";
		MessageString = "";
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

	if (TriangleBytesData) delete[] TriangleBytesData;
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
		SourceContext* Context = new SourceContext();

		uint NameLength = BytesToUnsignedIntegerLittleEndian(TriangleBytesData, Pos);
		Pos += 4;
		Context->Name = BytesToASCIIString(TriangleBytesData, Pos, NameLength);
		Pos += NameLength;

		int IndicesLength = BytesToUnsignedIntegerLittleEndian(TriangleBytesData, Pos);
		Pos += 4;

		Byte* Bytes = TriangleBytesData + Pos;
		Pos += ELEMENT_LENGTH * IndicesLength;

		Context->BytesData = Bytes;
		Context->TotalLength = ELEMENT_LENGTH * IndicesLength;
		Context->IndicesLength = IndicesLength;
		Context->TriangleNum = IndicesLength / 3;
		Context->CurrentFacePos = 0;
		Context->CurrentIndexPos = 0;
		Context->VertexData = VertexContextList[i];

		Context->FaceList.reserve(Context->TriangleNum);
		Context->EdgeList.clear();
		Context->EdgeFaceList.clear();
		Context->AdjacencyVertexFaceMap.clear();

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
	
	//IndexMap : actual vertex index in BytesData -> merged vertex  index 
	std::unordered_map<uint, uint>& IndexMap = Src->VertexData->IndexMap;
	std::unordered_map<uint, uint>::iterator it;
	uint actual_x = x, actual_y = y, actual_z = z;
	it = IndexMap.find(x);
	if (it != IndexMap.end()) x = it->second;
	it = IndexMap.find(y);
	if (it != IndexMap.end()) y = it->second;
	it = IndexMap.find(z);
	if (it != IndexMap.end()) z = it->second;

	Index ix(x, actual_x);
	Index iy(y, actual_y);
	Index iz(z, actual_z);

	Edge e1(ix, iy);
	Edge e2(iy, iz);
	Edge e3(iz, ix);

	if (AllHardEdge) {
		e1.IsHardEdge = true;
		e2.IsHardEdge = true;
		e3.IsHardEdge = true;
	}

	FacePair adj = FacePair();
	adj.face1 = FaceId;
	adj.set1 = true;

	std::unordered_map<Edge, FacePair>::iterator it1;
	it1 = Src->EdgeFaceList.find(e1);
	if (it1 != Src->EdgeFaceList.end())
	{
#if DEBUG_1
		if (it1->second.set2 == true)
		{
			ErrorString += "A edge has more than 2 face. Face Id : " + std::to_string(FaceId) + "\n";
			ErrorString += "Edge 1: " + std::to_string(e1.v1.actual_value) + ", " + std::to_string(e1.v2.actual_value) + " H " + std::to_string(e1.hash) + "\n";
			ErrorString += "Edge 2: " + std::to_string(it1->first.v1.actual_value) + ", " + std::to_string(it1->first.v2.actual_value) + " H " + std::to_string(it1->first.hash) + "\n";
		}
		else
#endif
		{
			it1->second.face2 = FaceId;
			it1->second.set2 = true;
		}
		e1.id = it1->first.id;

#if DEBUG_2
		ErrorString += "Write Edge: Found " + std::to_string(e1.v1.actual_value) + ", " + std::to_string(e1.v2.actual_value) + " | " + std::to_string(it1->second.face1) + ", " + std::to_string(it1->second.face2) + "\n";
#endif
	}
	else
	{
		e1.id = Src->EdgeList.size();

		Src->EdgeFaceList.insert(std::unordered_map<Edge, FacePair>::value_type(e1, adj));
		Src->EdgeList.push_back(e1);
		
#if DEBUG_2
		ErrorString += "Write Edge: NOTFound " + std::to_string(e1.v1.actual_value) + ", " + std::to_string(e1.v2.actual_value) + " | " + std::to_string(adj.face1) + ", " + std::to_string(adj.face2) + "\n";
#endif
	}

	std::unordered_map<Edge, FacePair>::iterator it2 = Src->EdgeFaceList.find(e2);
	if (it2 != Src->EdgeFaceList.end())
	{
#if DEBUG_1
		if (it2->second.set2 == true)
		{
			ErrorString += "A edge has more than 2 face. Face Id : " + std::to_string(FaceId) + "\n";
			ErrorString += "Edge 1: " + std::to_string(e2.v1.actual_value) + ", " + std::to_string(e2.v2.actual_value) + " H " + std::to_string(e2.hash) + "\n";
			ErrorString += "Edge 2: " + std::to_string(it2->first.v1.actual_value) + ", " + std::to_string(it2->first.v2.actual_value) + " H " + std::to_string(it2->first.hash) + "\n";
		}
		else
#endif
		{
			it2->second.face2 = FaceId;
			it2->second.set2 = true;
		}
		e2.id = it2->first.id;

#if DEBUG_2
		ErrorString += "Write Edge: Found " + std::to_string(e2.v1.actual_value) + ", " + std::to_string(e2.v2.actual_value) + " | " + std::to_string(it2->second.face1) + ", " + std::to_string(it2->second.face2) + "\n";
#endif
	}
	else
	{
		e2.id = Src->EdgeList.size();

		Src->EdgeFaceList.insert(std::unordered_map<Edge, FacePair>::value_type(e2, adj));
		Src->EdgeList.push_back(e2);

#if DEBUG_2
		ErrorString += "Write Edge: NOTFound " + std::to_string(e2.v1.actual_value) + ", " + std::to_string(e2.v2.actual_value) + " | " + std::to_string(adj.face1) + ", " + std::to_string(adj.face2) + "\n";
#endif
	}

	std::unordered_map<Edge, FacePair>::iterator it3 = Src->EdgeFaceList.find(e3);
	if (it3 != Src->EdgeFaceList.end())
	{
#if DEBUG_1
		if (it3->second.set2 == true)
		{
			ErrorString += "A edge has more than 2 face. Face Id : " + std::to_string(FaceId) + "\n";
			ErrorString += "Edge 1: " + std::to_string(e3.v1.actual_value) + ", " + std::to_string(e3.v2.actual_value) + " H " + std::to_string(e3.hash) + "\n";
			ErrorString += "Edge 2: " + std::to_string(it3->first.v1.actual_value) + ", " + std::to_string(it3->first.v2.actual_value) + " H " + std::to_string(it3->first.hash) + "\n";
		}
		else
#endif
		{
			it3->second.face2 = FaceId;
			it3->second.set2 = true;
		}
		e3.id = it3->first.id;

#if DEBUG_2
		ErrorString += "Write Edge: Found " + std::to_string(e3.v1.actual_value) + ", " + std::to_string(e3.v2.actual_value) + " | " + std::to_string(it3->second.face1) + ", " + std::to_string(it3->second.face2) + "\n";
#endif
	}
	else
	{
		e3.id = Src->EdgeList.size();

		Src->EdgeFaceList.insert(std::unordered_map<Edge, FacePair>::value_type(e3, adj));
		Src->EdgeList.push_back(e3);

#if DEBUG_2
		ErrorString += "Write Edge: NOT Found " + std::to_string(e3.v1.actual_value) + ", " + std::to_string(e3.v2.actual_value) + " | " + std::to_string(adj.face1) + ", " + std::to_string(adj.face2) + "\n";
#endif
	}

	Float3 px = Src->VertexData->VertexList[ix.value].Get();
	Float3 py = Src->VertexData->VertexList[iy.value].Get();
	Float3 pz = Src->VertexData->VertexList[iz.value].Get();
	Float3 Normal = CalculateNormal(px, py, pz);
	Float3 Center = (px + py + pz) / 3.0f;

	Face NewFace = Face(ix, iy, iz, e1.id, e2.id, e3.id, Normal, Center);
	Src->FaceList.emplace_back(NewFace);

	//{
	//	std::cout << NewFace.x.value << "," << NewFace.y.value << std::endl;
	//	std::cout << Src->EdgeList[NewFace.xy].v1.value << "," << Src->EdgeList[NewFace.xy].v2.value << std::endl;
	//}
	//{
	//	std::cout << NewFace.y.value << "," << NewFace.z.value << std::endl;
	//	std::cout << Src->EdgeList[NewFace.yz].v1.value << "," << Src->EdgeList[NewFace.yz].v2.value << std::endl;
	//}
	//{
	//	std::cout << NewFace.z.value << "," << NewFace.x.value << std::endl;
	//	std::cout << Src->EdgeList[NewFace.zx].v1.value << "," << Src->EdgeList[NewFace.zx].v2.value << std::endl;
	//}
	//std::cout << "=================================" << std::endl;

	std::unordered_map<uint, std::set<uint>>::iterator xFind = Src->AdjacencyVertexFaceMap.find(ix.value);
	if (xFind != Src->AdjacencyVertexFaceMap.end())
	{
		xFind->second.insert(Src->CurrentFacePos);
	}
	else
	{
		Src->AdjacencyVertexFaceMap.insert(std::unordered_map<uint, std::set<uint>>::value_type(ix.value, std::set<uint>()));
		Src->AdjacencyVertexFaceMap[ix.value].insert(Src->CurrentFacePos);
	}

	std::unordered_map<uint, std::set<uint>>::iterator yFind = Src->AdjacencyVertexFaceMap.find(iy.value);
	if (yFind != Src->AdjacencyVertexFaceMap.end())
	{
		yFind->second.insert(Src->CurrentFacePos);
	}
	else
	{
		Src->AdjacencyVertexFaceMap.insert(std::unordered_map<uint, std::set<uint>>::value_type(iy.value, std::set<uint>()));
		Src->AdjacencyVertexFaceMap[iy.value].insert(Src->CurrentFacePos);
	}

	std::unordered_map<uint, std::set<uint>>::iterator zFind = Src->AdjacencyVertexFaceMap.find(iz.value);
	if (zFind != Src->AdjacencyVertexFaceMap.end())
	{
		zFind->second.insert(Src->CurrentFacePos);
	}
	else
	{
		Src->AdjacencyVertexFaceMap.insert(std::unordered_map<uint, std::set<uint>>::value_type(iz.value, std::set<uint>()));
		Src->AdjacencyVertexFaceMap[iz.value].insert(Src->CurrentFacePos);
	}


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
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || IsWorking())
	{
		ErrorString += "Gerneration has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;

		WRITE_MESSAGE("Mesh Name : ", TriangleContextList[i]->Name);
		WRITE_MESSAGE_DIGIT("VertexLength: ", TriangleContextList[i]->VertexData->VertexList.size());
		WRITE_MESSAGE_DIGIT("FaceLength: ", TriangleContextList[i]->FaceList.size());
		WRITE_MESSAGE_DIGIT("EdgeLength: ", TriangleContextList[i]->EdgeList.size());

		TriangleContextList[i]->FaceNormalMap.resize(TriangleContextList[i]->FaceList.size());

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


bool SourceContext::GetAdjFaceByEdge(uint TargetFaceId, uint TargetEdgeId, bool IgnoreHardEdge, uint* OutFaceId)
{
	bool Result = false;
	const Edge* TargetEdge = &EdgeList[TargetEdgeId];
	if (!TargetEdge->IsHardEdge || IgnoreHardEdge)
	{
		std::unordered_map<Edge, FacePair>::iterator it = EdgeFaceList.find(*TargetEdge);
		if (it != EdgeFaceList.end())
		{
			uint AdjFaceId = 0;
			if (it->second.GetOppositeFace(TargetFaceId, &AdjFaceId))
			{
				*OutFaceId = AdjFaceId;
				Result = true;
			}
		}
	}

	return Result;
}

Float3 SourceContext::CalculateVertexNormalOnFace(uint TargetFaceId, uint TargetVertexId)
{
	if (AdjacencyVertexFaceMap.find(TargetVertexId) == AdjacencyVertexFaceMap.end()) 
		return FaceList[TargetFaceId].normal;

	uint EdgesId[2] = { 0, 0 };
	FaceList[TargetFaceId].GetEdgesIndexByVertex(TargetVertexId, &EdgesId[0], &EdgesId[1]);

	std::unordered_map<uint, bool> FaceSet;
	std::set<uint>& AdjFaceSet = AdjacencyVertexFaceMap[TargetVertexId];
	for (std::set<uint>::iterator i = AdjFaceSet.begin(); i != AdjFaceSet.end(); i++)
	{
		FaceSet.insert(std::pair<uint, bool>(*i, *i == TargetFaceId));
	}

	for (int i = 0; i < 2; i++) {
		uint CurrentEdgeId = EdgesId[i];
		uint CurrentFaceId = TargetFaceId;
		uint AdjFaceId = 0;
		while (GetAdjFaceByEdge(CurrentFaceId, CurrentEdgeId, false, &AdjFaceId))
		{
			FaceSet[AdjFaceId] = true;
			CurrentFaceId = AdjFaceId;
			CurrentEdgeId = FaceList[CurrentFaceId].GetOppositeEdge(TargetVertexId, CurrentEdgeId);

			if (AdjFaceId == TargetFaceId) break;
		};
	}


	std::vector<Float3> Normals;
	std::vector<float> Weights;
	for (std::unordered_map<uint, bool>::iterator i = FaceSet.begin(); i != FaceSet.end(); i++)
	{
		Float3 Nor = FaceList[i->first].normal;
		float Wei = 1.0f;
		float Num = 0.0f;
		for (std::unordered_map<uint, bool>::iterator j = FaceSet.begin(); j != FaceSet.end(); j++)
		{
			if (Dot(Nor, FaceList[j->first].normal) >= 0.99998f)
			{
				Num += 1.0f;
			}
		}
		Normals.push_back(Nor);
		Weights.push_back(i->second ? Wei / Num : 0.0f);
	}

	Float3 Result = 0.0f;
	for (int i = 0; i < Normals.size(); i++)
	{
		Result = Result + (Normals[i] * Weights[i]);
	}

	return Normalize(Result);
}


void* AdjacencyProcesser::RunFunc2(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;
	Face& CurrentFace = Src->FaceList[Src->CurrentFacePos];

	FaceNormal NormalSet;

	NormalSet.normalvx = Src->CalculateVertexNormalOnFace(Src->CurrentFacePos, CurrentFace.x.value);
	NormalSet.normalvy = Src->CalculateVertexNormalOnFace(Src->CurrentFacePos, CurrentFace.y.value);
	NormalSet.normalvz = Src->CalculateVertexNormalOnFace(Src->CurrentFacePos, CurrentFace.z.value);

	Src->FaceNormalMap[Src->CurrentFacePos] = NormalSet;


	Src->CurrentFacePos++;

	double Step = 1.0 / Src->FaceList.size();
	*OutProgressPerRun = Step;

	if (Src->CurrentFacePos >= Src->FaceList.size())
		return (void*)Src;
	else
		return nullptr;
}





bool AdjacencyProcesser::GetReadyForGenerateMeshletLayer01Data()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || IsWorking())
	{
		ErrorString += "Gerneration has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;

		TriangleContextList[i]->MeshletLayer0Data.Init(TriangleContextList[i]->VertexData->VertexList.size(), TriangleContextList[i]->FaceList.size());
		TriangleContextList[i]->MeshletLayer1Data.Init(TriangleContextList[i]->VertexData->VertexList.size(), TriangleContextList[i]->FaceList.size());

		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunGenerateMeshletLayer01Data, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}



void* AdjacencyProcesser::RunGenerateMeshletLayer01Data(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;
	VertexContext* VertexData = Src->VertexData;
	MeshOpt& CurrentMeshletData0 = Src->MeshletLayer0Data;
	MeshOpt& CurrentMeshletData1 = Src->MeshletLayer1Data;

	Face& CurrentFace = Src->FaceList[Src->CurrentFacePos];

	Vertex3& Vertex0 = VertexData->VertexList[CurrentFace.x.value];
	Vertex3& Vertex1 = VertexData->VertexList[CurrentFace.y.value];
	Vertex3& Vertex2 = VertexData->VertexList[CurrentFace.z.value];

	Float3 V0 = Vertex0.Get();
	Float3 V1 = Vertex1.Get();
	Float3 V2 = Vertex2.Get();

	Float3 V10 = V1 - V0;
	Float3 V20 = V2 - V0;
	Float3 Normal = Cross(V10, V20);
	float Area = Length(Normal) * 0.5f;

	std::set<unsigned int> indexes;
	indexes.insert(CurrentFace.x.value);
	indexes.insert(CurrentFace.y.value);
	indexes.insert(CurrentFace.z.value);
	MeshOptTriangle NewCell = MeshOptTriangle(CurrentFace.center, CurrentFace.normal, Area, &indexes);

	//find adjacency triangle
	std::unordered_map<uint, std::set<uint>>::iterator it = Src->AdjacencyVertexFaceMap.find(CurrentFace.x.value);
	if (it != Src->AdjacencyVertexFaceMap.end())
	{
		std::set<uint>& AdjFaceList = it->second;
		for (std::set<uint>::iterator i = AdjFaceList.begin(); i != AdjFaceList.end(); i++)
		{
			CurrentMeshletData0.vertices[CurrentFace.x.value].neighborTriangles.insert(*i);
			CurrentMeshletData1.vertices[CurrentFace.x.value].neighborTriangles.insert(*i);
		}
		CurrentMeshletData0.live_triangles[CurrentFace.x.value] = AdjFaceList.size();
		CurrentMeshletData1.live_triangles[CurrentFace.x.value] = AdjFaceList.size();
	}
	else
	{
		ErrorString += "Cannot find vertex in AdjacencyVertexFaceMap. Current Vertex: " + std::to_string(CurrentFace.x.value) + "\n";
	}

	it = Src->AdjacencyVertexFaceMap.find(CurrentFace.y.value);
	if (it != Src->AdjacencyVertexFaceMap.end())
	{
		std::set<uint>& AdjFaceList = it->second;
		for (std::set<uint>::iterator i = AdjFaceList.begin(); i != AdjFaceList.end(); i++)
		{
			CurrentMeshletData0.vertices[CurrentFace.y.value].neighborTriangles.insert(*i);
			CurrentMeshletData1.vertices[CurrentFace.y.value].neighborTriangles.insert(*i);
		}
		CurrentMeshletData0.live_triangles[CurrentFace.y.value] = AdjFaceList.size();;
		CurrentMeshletData1.live_triangles[CurrentFace.y.value] = AdjFaceList.size();;
	}
	else
	{
		ErrorString += "Cannot find vertex in AdjacencyVertexFaceMap. Current Vertex: " + std::to_string(CurrentFace.y.value) + "\n";
	}

	it = Src->AdjacencyVertexFaceMap.find(CurrentFace.z.value);
	if (it != Src->AdjacencyVertexFaceMap.end())
	{
		std::set<uint>& AdjFaceList = it->second;
		for (std::set<uint>::iterator i = AdjFaceList.begin(); i != AdjFaceList.end(); i++)
		{
			CurrentMeshletData0.vertices[CurrentFace.z.value].neighborTriangles.insert(*i);
			CurrentMeshletData1.vertices[CurrentFace.z.value].neighborTriangles.insert(*i);
		}
		CurrentMeshletData0.live_triangles[CurrentFace.z.value] = AdjFaceList.size();
		CurrentMeshletData1.live_triangles[CurrentFace.z.value] = AdjFaceList.size();
	}
	else
	{
		ErrorString += "Cannot find vertex in AdjacencyVertexFaceMap. Current Vertex: " + std::to_string(CurrentFace.z.value) + "\n";
	}

	CurrentMeshletData0.mesh_area = CurrentMeshletData0.mesh_area + NewCell.Area;
	CurrentMeshletData0.triangles.push_back(NewCell);
	CurrentMeshletData0.kdindices[Src->CurrentFacePos] = unsigned int(Src->CurrentFacePos);

	CurrentMeshletData1.mesh_area = CurrentMeshletData1.mesh_area + NewCell.Area;
	CurrentMeshletData1.triangles.push_back(NewCell);
	CurrentMeshletData1.kdindices[Src->CurrentFacePos] = unsigned int(Src->CurrentFacePos);

	Src->CurrentFacePos++;

	double Step = 1.0 / Src->FaceList.size();
	*OutProgressPerRun = Step;

	if (Src->CurrentFacePos >= Src->FaceList.size())
		return (void*)Src;
	else
		return nullptr;
}



bool AdjacencyProcesser::GetReadyGenerateMeshletLayer0()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || IsWorking())
	{
		ErrorString += "Gerneration has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;

		size_t MaxTrianglesNumInMeshlet = MeshletLayer1MaxCellNum;
		TriangleContextList[i]->MeshletLayer0Data.GetReady(MaxTrianglesNumInMeshlet, TriangleContextList[i]->FaceList.size(), TriangleContextList[i]->VertexData->VertexList.size(), MeshletNormalWeight);

		WRITE_MESSAGE_DIGIT("Max Cell Num In Meshlet: ", MaxTrianglesNumInMeshlet);
		WRITE_MESSAGE_DIGIT("Current Cell Num: ", TriangleContextList[i]->MeshletLayer0Data.triangles.size());
		WRITE_MESSAGE_DIGIT("ExpectedRadius: ", TriangleContextList[i]->MeshletLayer0Data.meshlet_expected_radius);

		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunGenerateMeshletLayer0, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}

void* AdjacencyProcesser::RunGenerateMeshletLayer0(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	if (Src->MeshletLayer0Data.RunStep(OutProgressPerRun, false)) {
		return (void*)Src;
	}
	else {
		return nullptr;
	}

}



bool AdjacencyProcesser::GetReadyForSerializeMeshletLayer0()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || IsWorking())
	{
		ErrorString += "Gerneration has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;

		WRITE_MESSAGE("Mesh Name : ", TriangleContextList[i]->Name);
		WRITE_MESSAGE_DIGIT("Layer 0 Meshlet Num: ", TriangleContextList[i]->MeshletLayer0Data.GetMeshletSize());

		//for (uint j = 0; j < TriangleContextList[i]->MeshletLayer0Data.GetMeshletSize(); j++)
		//{
		//	std::cout << TriangleContextList[i]->MeshletLayer0Data.meshlets_list[j].triangle_offset << std::endl;
		//	std::cout << TriangleContextList[i]->MeshletLayer0Data.meshlets_list[j].triangle_count << std::endl;
		//	std::cout << "---------------------------------------------------------------" << std::endl;
		//}

		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunFuncForSerializeMeshletLayer0, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}

void* AdjacencyProcesser::RunFuncForSerializeMeshletLayer0(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	const MeshOpt& MeshletLayer0Data = Src->MeshletLayer0Data;
	if (MeshletLayer0Data.GetMeshletSize() > Src->CurrentIndexPos) {
		const meshopt_Meshlet& CurrentMeshlet = MeshletLayer0Data.meshlets_list[Src->CurrentIndexPos];
		uint CurrentMeshletIndex = Src->CurrentIndexPos;

		if (CurrentMeshlet.triangle_count > Src->CurrentFacePos) {
			uint CurrentFaceIndex = MeshletLayer0Data.meshlet_triangles[size_t(CurrentMeshlet.triangle_offset) + size_t(Src->CurrentFacePos)];

			Face& DestFace = Src->FaceList[CurrentFaceIndex];
			if (DestFace.meshletId[0] == -1) DestFace.meshletId[0] = CurrentMeshletIndex;

			*OutProgressPerRun = 0.0;
			Src->CurrentFacePos++;
		}
		else
		{
			Src->CurrentIndexPos++;
			*OutProgressPerRun = 1.0 / MeshletLayer0Data.GetMeshletSize();

			Src->CurrentFacePos = 0;
		}
	}


	if (Src->CurrentIndexPos >= MeshletLayer0Data.GetMeshletSize())
		return (void*)Src;
	else
		return nullptr;

}


bool AdjacencyProcesser::GetReadyGenerateMeshletLayer1()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || IsWorking())
	{
		ErrorString += "Gerneration has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;

		size_t MaxTrianglesNumInMeshlet = MeshletLayer1MaxCellNum;
		TriangleContextList[i]->MeshletLayer1Data.GetReady(MaxTrianglesNumInMeshlet, TriangleContextList[i]->FaceList.size(), TriangleContextList[i]->VertexData->VertexList.size(), MeshletNormalWeight);

		WRITE_MESSAGE_DIGIT("Max Cell Num In Meshlet: ", MaxTrianglesNumInMeshlet);
		WRITE_MESSAGE_DIGIT("Current Cell Num: ", TriangleContextList[i]->MeshletLayer1Data.triangles.size());
		WRITE_MESSAGE_DIGIT("ExpectedRadius: ", TriangleContextList[i]->MeshletLayer1Data.meshlet_expected_radius);

		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunGenerateMeshletLayer1, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}

void* AdjacencyProcesser::RunGenerateMeshletLayer1(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	if (Src->MeshletLayer1Data.RunStep(OutProgressPerRun, true)) {
		return (void*)Src;
	}
	else {
		return nullptr;
	}

}


bool AdjacencyProcesser::GetReadyForSerializeMeshletLayer1()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || IsWorking())
	{
		ErrorString += "Gerneration has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;
		
		WRITE_MESSAGE("Mesh Name : ", TriangleContextList[i]->Name);
		WRITE_MESSAGE_DIGIT("Layer 1 Meshlet Num: ", TriangleContextList[i]->MeshletLayer1Data.GetMeshletSize());


		//for (uint j = 0; j < TriangleContextList[i]->MeshletLayer1Data.GetMeshletSize(); j++)
		//{
		//	std::cout << TriangleContextList[i]->MeshletLayer1Data.meshlets_list[j].triangle_offset << std::endl;
		//	std::cout << TriangleContextList[i]->MeshletLayer1Data.meshlets_list[j].triangle_count << std::endl;
		//	std::cout << "---------------------------------------------------------------" << std::endl;
		//}
		

		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunFuncForSerializeMeshletLayer1, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}

void* AdjacencyProcesser::RunFuncForSerializeMeshletLayer1(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	const MeshOpt& MeshletLayer1Data = Src->MeshletLayer1Data;
	if (MeshletLayer1Data.GetMeshletSize() > Src->CurrentIndexPos) {
		const meshopt_Meshlet& CurrentMeshlet = MeshletLayer1Data.meshlets_list[Src->CurrentIndexPos];
		uint CurrentMeshletIndex = Src->CurrentIndexPos;

		if (CurrentMeshlet.triangle_count > Src->CurrentFacePos) {
			uint CurrentFaceIndex = MeshletLayer1Data.meshlet_triangles[size_t(CurrentMeshlet.triangle_offset) + size_t(Src->CurrentFacePos)]; 

			Face& DestFace = Src->FaceList[CurrentFaceIndex];
			if (DestFace.meshletId[1] == -1) DestFace.meshletId[1] = CurrentMeshletIndex;

			*OutProgressPerRun = 0.0;
			Src->CurrentFacePos++;
		}
		else
		{
			Src->CurrentIndexPos++;
			*OutProgressPerRun = 1.0 / MeshletLayer1Data.GetMeshletSize();

			Src->CurrentFacePos = 0;
		}
	}

	
	if (Src->CurrentIndexPos >= MeshletLayer1Data.GetMeshletSize())
		return (void*)Src;
	else
		return nullptr;

}








bool AdjacencyProcesser::GetReadyForGenerateMeshletLayer2Data()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || IsWorking())
	{
		ErrorString += "Gerneration has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;

		TriangleContextList[i]->MeshletLayer2Data.Init(TriangleContextList[i]->VertexData->VertexList.size(), TriangleContextList[i]->MeshletLayer1Data.GetMeshletSize());


		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunGenerateMeshletLayer2Data, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}



void* AdjacencyProcesser::RunGenerateMeshletLayer2Data(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;
	MeshOpt& Layer2MeshletData = Src->MeshletLayer2Data;
	
	const meshopt_Meshlet CurrentFace = Src->MeshletLayer1Data.meshlets_list[Src->CurrentFacePos];

	std::set<unsigned int> indexes;
	for (int i = 0; i < CurrentFace.vertex_count; i++)
	{
		unsigned int index = Src->MeshletLayer1Data.meshlet_vertices[size_t(CurrentFace.vertex_offset) + size_t(i)];
		if (Src->MeshletLayer1Data.vertex_meshlet_occupy_flag[index] > 1)
		{
			indexes.insert(index);
		}
	}
	MeshOptTriangle NewCell = MeshOptTriangle(CurrentFace.Center, CurrentFace.Normal, CurrentFace.Area, &indexes);

	//find adjacency meshlet
	for (std::set<unsigned int>::iterator it = indexes.begin(); it != indexes.end(); it++)
	{
		unsigned int index = *it;
		std::unordered_map<uint, std::set<uint>>::iterator j = Src->AdjacencyVertexFaceMap.find(index);
		if (j != Src->AdjacencyVertexFaceMap.end())
		{
			std::set<uint>& AdjFaceList = j->second;
			std::set<uint> MeshletIdList;
			for (std::set<uint>::iterator i = AdjFaceList.begin(); i != AdjFaceList.end(); i++)
			{
				MeshletIdList.insert(Src->FaceList[*i].meshletId[1]);
			}
			for (std::set<uint>::iterator i = MeshletIdList.begin(); i != MeshletIdList.end(); i++)
			{
				Layer2MeshletData.vertices[index].neighborTriangles.insert(*i);
			}
			Layer2MeshletData.live_triangles[index] = MeshletIdList.size();
		}

	}
	
	Layer2MeshletData.mesh_area = Layer2MeshletData.mesh_area + NewCell.Area;
	Layer2MeshletData.triangles.push_back(NewCell);
	Layer2MeshletData.kdindices[Src->CurrentFacePos] = unsigned int(Src->CurrentFacePos);


	*OutProgressPerRun = 1.0 / Src->MeshletLayer1Data.GetMeshletSize();
	Src->CurrentFacePos++;

	if (Src->CurrentFacePos >= Src->MeshletLayer1Data.GetMeshletSize()) {
		return (void*)Src;
	}
	else
		return nullptr;
}


bool AdjacencyProcesser::GetReadyGenerateMeshletLayer2()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || IsWorking())
	{
		ErrorString += "GetReady5 has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;

		size_t MaxTrianglesNumInMeshlet = MeshletLayer2MaxCellNum;
		TriangleContextList[i]->MeshletLayer2Data.GetReady(MaxTrianglesNumInMeshlet, TriangleContextList[i]->MeshletLayer1Data.GetMeshletSize(), TriangleContextList[i]->VertexData->VertexList.size(), 0.0f);

		WRITE_MESSAGE_DIGIT("Max Cell Num In Meshlet: ", MaxTrianglesNumInMeshlet);
		WRITE_MESSAGE_DIGIT("Current Cell Num: ", TriangleContextList[i]->MeshletLayer2Data.triangles.size());
		WRITE_MESSAGE_DIGIT("ExpectedRadius: ", TriangleContextList[i]->MeshletLayer2Data.meshlet_expected_radius);

		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunGenerateMeshletLayer2, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}


void* AdjacencyProcesser::RunGenerateMeshletLayer2(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	if (Src->MeshletLayer2Data.RunStep(OutProgressPerRun, true)) {
		return (void*)Src;
	}
	else {
		return nullptr;
	}
}


bool AdjacencyProcesser::GetReadyForSerializeMeshletLayer2()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || IsWorking())
	{
		ErrorString += "Gerneration has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;

		WRITE_MESSAGE("Mesh Name : ", TriangleContextList[i]->Name);
		WRITE_MESSAGE_DIGIT("Layer 2 Meshlet Num: ", TriangleContextList[i]->MeshletLayer2Data.GetMeshletSize());

		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunFuncForSerializeMeshletLayer2, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}


void* AdjacencyProcesser::RunFuncForSerializeMeshletLayer2(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	double Step = 0.0;

	if (Src->MeshletLayer2Data.GetMeshletSize() > Src->CurrentIndexPos) {
		meshopt_Meshlet& CurrentMeshlet = Src->MeshletLayer2Data.meshlets_list[Src->CurrentIndexPos];

		if (CurrentMeshlet.triangle_count > Src->CurrentFacePos) {
			unsigned int Index = Src->MeshletLayer2Data.meshlet_triangles[size_t(CurrentMeshlet.triangle_offset) +size_t(Src->CurrentFacePos)];
			meshopt_Meshlet& CurrentCell = Src->MeshletLayer1Data.meshlets_list[Index];

			for (int i = 0; i < CurrentCell.triangle_count; i++)
			{
				unsigned int FaceIndex = Src->MeshletLayer1Data.meshlet_triangles[size_t(CurrentCell.triangle_offset)+ size_t(i)];
				if (Src->FaceList[FaceIndex].meshletId[2] == -1)
					Src->FaceList[FaceIndex].meshletId[2] = Src->CurrentIndexPos;
				
			}

			Src->CurrentFacePos++;
		}
		else
		{
			Src->CurrentIndexPos++;
			Src->CurrentFacePos = 0;
			Step = 1.0 / Src->MeshletLayer2Data.GetMeshletSize();
		}
	}


	*OutProgressPerRun = Step;

	if (Src->CurrentIndexPos >= Src->MeshletLayer2Data.GetMeshletSize())
		return (void*)Src;
	else
		return nullptr;
}







bool AdjacencyProcesser::GetReadyForSerializeExportData()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || IsWorking())
	{
		ErrorString += "Gerneration has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;

		TriangleContextList[i]->ClearExportData();
		
		TriangleContextList[i]->EXPORTFaceList.resize(TriangleContextList[i]->FaceList.size());
		TriangleContextList[i]->MeshletVertexMap.resize(TriangleContextList[i]->MeshletLayer0Data.GetMeshletSize());

		WRITE_MESSAGE("Serializing: ", TriangleContextList[i]->Name);

		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunFuncSerializeExportData, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}


void* AdjacencyProcesser::RunFuncSerializeExportData(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	MeshOpt& MeshletLayer0Data = Src->MeshletLayer0Data;
	if (MeshletLayer0Data.GetMeshletSize() > Src->CurrentIndexPos) {
		meshopt_Meshlet& CurrentMeshlet = MeshletLayer0Data.meshlets_list[Src->CurrentIndexPos];

		if (Src->CurrentFacePos == 0) {
			CurrentMeshlet.vertex_offset = Src->EXPORTRepeatedVertexList.size();
			CurrentMeshlet.vertex_count = 0;
		}

		if (CurrentMeshlet.triangle_count > Src->CurrentFacePos) {
			uint  CurrentFaceIndex = MeshletLayer0Data.meshlet_triangles[CurrentMeshlet.triangle_offset + Src->CurrentFacePos];

			const Face& DestFace = Src->FaceList[CurrentFaceIndex];
			const Vertex3& V1 = Src->VertexData->VertexList[DestFace.x.value];
			const Vertex3& V2 = Src->VertexData->VertexList[DestFace.y.value];
			const Vertex3& V3 = Src->VertexData->VertexList[DestFace.z.value];

			Src->EdgeUsedMap[DestFace.xy] = false;
			Src->EdgeUsedMap[DestFace.yz] = false;
			Src->EdgeUsedMap[DestFace.zx] = false;

			EXPORTVertex ExV1(V1.Get(), DestFace.x.value);
			EXPORTVertex ExV2(V2.Get(), DestFace.y.value);
			EXPORTVertex ExV3(V3.Get(), DestFace.z.value);
			std::map<uint, uint>& VertexSet = Src->MeshletVertexMap[Src->CurrentIndexPos];
			uint V1Index = 0;
			if (VertexSet.find(DestFace.x.value) == VertexSet.end()) {
				V1Index = Src->EXPORTRepeatedVertexList.size();
				Src->EXPORTRepeatedVertexList.push_back(ExV1);
				CurrentMeshlet.vertex_count++;
				VertexSet.insert(std::pair(DestFace.x.value, V1Index));
			}
			else
			{
				V1Index = VertexSet[DestFace.x.value];
			}
			
			uint V2Index = 0;
			if (VertexSet.find(DestFace.y.value) == VertexSet.end()) {
				V2Index = Src->EXPORTRepeatedVertexList.size();
				Src->EXPORTRepeatedVertexList.push_back(ExV2);
				CurrentMeshlet.vertex_count++;
				VertexSet.insert(std::pair(DestFace.y.value, V2Index));
			}
			else
			{
				V2Index = VertexSet[DestFace.y.value];
			}

			uint V3Index = 0;
			if (VertexSet.find(DestFace.z.value) == VertexSet.end()) {
				V3Index = Src->EXPORTRepeatedVertexList.size();
				Src->EXPORTRepeatedVertexList.push_back(ExV3);
				CurrentMeshlet.vertex_count++;
				VertexSet.insert(std::pair(DestFace.z.value, V3Index));
			}
			else
			{
				V3Index = VertexSet[DestFace.z.value];
			}

			V1Index -= CurrentMeshlet.vertex_offset;
			V2Index -= CurrentMeshlet.vertex_offset;
			V3Index -= CurrentMeshlet.vertex_offset;
			assert(V1Index < MeshletLayer0Data.max_triangles * 3 && V1Index >= 0);
			assert(V2Index < MeshletLayer0Data.max_triangles * 3 && V2Index >= 0);
			assert(V3Index < MeshletLayer0Data.max_triangles * 3 && V3Index >= 0);

			uint ExFaceIndex = CurrentMeshlet.triangle_offset + Src->CurrentFacePos;
			EXPORTFace ExFace;
			ExFace.v012 = EncodeVertexIndex(V1Index, V2Index, V3Index);

			FaceNormal& NormalSet = Src->FaceNormalMap[CurrentFaceIndex];
			ExFace.PackNormalData0 = EncodeNormalsToUint3(DestFace.normal, NormalSet.normalvx);
			ExFace.PackNormalData1 = EncodeNormalsToUint3(NormalSet.normalvy,  NormalSet.normalvz);

			/*
			Float3 Normal;
			Float3 V1Normal;
			DecodeUint3ToNormals(ExFace.PackNormalData0, &Normal, &V1Normal);

			const float EPSS = 0.00005f;
			if (std::abs(Normal.x - DestFace.normal.x) >= EPSS || std::abs(Normal.y - DestFace.normal.y) >= EPSS || std::abs(Normal.z - DestFace.normal.z) >= EPSS
				|| std::abs(V1Normal.x - NormalSet.normalvx.x) >= EPSS || std::abs(V1Normal.y - NormalSet.normalvx.y) >= EPSS || std::abs(V1Normal.z - NormalSet.normalvx.z) >= EPSS) {
				std::cout << "Normal(" << DestFace.normal.x << "," << DestFace.normal.y << "," << DestFace.normal.z << ")"
					<< " V1Normal(" << NormalSet.normalvx.x << "," << NormalSet.normalvx.y << "," << NormalSet.normalvx.z << ")" << std::endl;

				std::cout << "UNormal(" << Normal.x << "," << Normal.y << "," << Normal.z << ")"
					<< " UV1Normal(" << V1Normal.x << "," << V1Normal.y << "," << V1Normal.z << ")" << std::endl;

				std::cout << "E(" << std::abs(Normal.x - DestFace.normal.x) << "," << std::abs(Normal.y - DestFace.normal.y) << "," << std::abs(Normal.z - DestFace.normal.z)
					<< ")" << "E(" << std::abs(V1Normal.x - NormalSet.normalvx.x) << "," << std::abs(V1Normal.y - NormalSet.normalvx.y) << "," << std::abs(V1Normal.z - NormalSet.normalvx.z) << ")" << std::endl << std::endl;
			}*/


			Src->EXPORTFaceList[ExFaceIndex] = ExFace;
			Src->MeshletFaceMap.insert(std::pair(CurrentFaceIndex, ExFaceIndex));

			*OutProgressPerRun = 0.0;
			Src->CurrentFacePos++;
		}
		else
		{
			Src->CurrentIndexPos++;
			*OutProgressPerRun = 1.0 / MeshletLayer0Data.GetMeshletSize();

			Src->CurrentFacePos = 0;
		}
	}

	if (Src->CurrentIndexPos >= MeshletLayer0Data.GetMeshletSize())
		return (void*)Src;
	else
		return nullptr;

}


bool AdjacencyProcesser::GetReadyForSerializeExportData2()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || IsWorking())
	{
		ErrorString += "Gerneration has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;

		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunFuncSerializeExportData2, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}

void* AdjacencyProcesser::RunFuncSerializeExportData2(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	const MeshOpt& MeshletLayer0Data = Src->MeshletLayer0Data;
	if (MeshletLayer0Data.GetMeshletSize() > Src->CurrentIndexPos) {
		const meshopt_Meshlet& CurrentMeshlet = MeshletLayer0Data.meshlets_list[Src->CurrentIndexPos];

		if (CurrentMeshlet.triangle_count > Src->CurrentFacePos) {
			uint  CurrentFaceIndex = MeshletLayer0Data.meshlet_triangles[CurrentMeshlet.triangle_offset + Src->CurrentFacePos];

			EXPORTFace& DestExportFace = Src->EXPORTFaceList[Src->MeshletFaceMap[CurrentFaceIndex]];
			const Face& DestFace = Src->FaceList[CurrentFaceIndex];
			const Edge& E1 = Src->EdgeList[DestFace.xy];
			const Edge& E2 = Src->EdgeList[DestFace.yz];
			const Edge& E3 = Src->EdgeList[DestFace.zx];
			
			DestExportFace.UniqueIndex01 = DestFace.xy;
			DestExportFace.UniqueIndex12 = DestFace.yz;
			DestExportFace.UniqueIndex20 = DestFace.zx;

			DestExportFace.MeshletData[0] = DestFace.meshletId[0];
			DestExportFace.MeshletData[1] = DestFace.meshletId[1];
			DestExportFace.MeshletData[2] = DestFace.meshletId[2];

			bool IsE1Unique = true;
			if (Src->EdgeUsedMap.find(DestFace.xy) != Src->EdgeUsedMap.end())
			{
				IsE1Unique = !Src->EdgeUsedMap[DestFace.xy];
				if (Src->EdgeUsedMap[DestFace.xy] == false) Src->EdgeUsedMap[DestFace.xy] = true;
			}
			
			bool IsE2Unique = true;
			if (Src->EdgeUsedMap.find(DestFace.yz) != Src->EdgeUsedMap.end())
			{
				IsE2Unique = !Src->EdgeUsedMap[DestFace.yz];
				if (Src->EdgeUsedMap[DestFace.yz] == false) Src->EdgeUsedMap[DestFace.yz] = true;
			}

			bool IsE3Unique = true;
			if (Src->EdgeUsedMap.find(DestFace.zx) != Src->EdgeUsedMap.end())
			{
				IsE3Unique = !Src->EdgeUsedMap[DestFace.zx];
				if (Src->EdgeUsedMap[DestFace.zx] == false) Src->EdgeUsedMap[DestFace.zx] = true;
			}

			std::unordered_map<Edge, FacePair>::iterator Found = Src->EdgeFaceList.find(E1);
			if(Found != Src->EdgeFaceList.end())
			{
				uint E1AdjFaceIndex = 0;
				uint E1ExportAdjFaceIndex = 0;
				if (Found->second.GetOppositeFace(CurrentFaceIndex, &E1AdjFaceIndex))
				{
					E1ExportAdjFaceIndex = Src->MeshletFaceMap[E1AdjFaceIndex] + 1;
				}
				DestExportFace.edge01 = EncodeEdgeIndex(E1ExportAdjFaceIndex, E1.IsHardEdge, IsE1Unique);
			}
			else
			{
				std::cout << "Cannot find edge in EdgeFaceList : " << DestFace.xy << ", Face Id" << CurrentFaceIndex << std::endl;
			}

			Found = Src->EdgeFaceList.find(E2);
			if (Found != Src->EdgeFaceList.end())
			{
				uint E2AdjFaceIndex = 0;
				uint E2ExportAdjFaceIndex = 0;
				if (Found->second.GetOppositeFace(CurrentFaceIndex, &E2AdjFaceIndex))
				{
					E2ExportAdjFaceIndex = Src->MeshletFaceMap[E2AdjFaceIndex] + 1;
				}
				DestExportFace.edge12 = EncodeEdgeIndex(E2ExportAdjFaceIndex, E2.IsHardEdge, IsE2Unique);
			}
			else
			{
				std::cout << "Cannot find edge in EdgeFaceList : " << DestFace.yz << ", Face Id" << CurrentFaceIndex << std::endl;
			}

			Found = Src->EdgeFaceList.find(E3);
			if (Found != Src->EdgeFaceList.end())
			{
				uint E3AdjFaceIndex = 0;
				uint E3ExportAdjFaceIndex = 0;
				if (Found->second.GetOppositeFace(CurrentFaceIndex, &E3AdjFaceIndex))
				{
					E3ExportAdjFaceIndex = Src->MeshletFaceMap[E3AdjFaceIndex] + 1;
				}
				DestExportFace.edge20 = EncodeEdgeIndex(E3ExportAdjFaceIndex, E3.IsHardEdge, IsE3Unique);
			}
			else
			{
				std::cout << "Cannot find edge in EdgeFaceList : " << DestFace.zx << ", Face Id" << CurrentFaceIndex << std::endl;
			}

			*OutProgressPerRun = 0.0;
			Src->CurrentFacePos++;
		}
		else
		{
			Src->EXPORTMeshletList.push_back(EXPORTMeshlet(CurrentMeshlet.triangle_offset, CurrentMeshlet.triangle_count, CurrentMeshlet.vertex_offset, CurrentMeshlet.vertex_count));

			Src->CurrentIndexPos++;
			*OutProgressPerRun = 1.0 / MeshletLayer0Data.GetMeshletSize();

			Src->CurrentFacePos = 0;
		}
	}

	if (Src->CurrentIndexPos >= MeshletLayer0Data.GetMeshletSize())
		return (void*)Src;
	else
		return nullptr;

}


bool AdjacencyProcesser::GetReadyGenerateRenderData()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || IsWorking())
	{
		ErrorString += "Gerneration has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;
		
		if (TriangleContextList[i]->VertexData->DrawVertexList != nullptr) delete[] TriangleContextList[i]->VertexData->DrawVertexList;
		TriangleContextList[i]->VertexData->DrawVertexList = new DrawRawVertex[TriangleContextList[i]->FaceList.size() * 3];
		memset(TriangleContextList[i]->VertexData->DrawVertexList, 0, TriangleContextList[i]->FaceList.size() * 3);

		if (TriangleContextList[i]->DrawIndexList != nullptr) delete[] TriangleContextList[i]->DrawIndexList;
		TriangleContextList[i]->DrawIndexList = new DrawRawIndex[TriangleContextList[i]->FaceList.size() * 3];
		memset(TriangleContextList[i]->DrawIndexList, 0, TriangleContextList[i]->FaceList.size() * 3);

		if (TriangleContextList[i]->VertexData->DrawFaceNormalVertexList != nullptr) delete[] TriangleContextList[i]->VertexData->DrawFaceNormalVertexList;
		TriangleContextList[i]->VertexData->DrawFaceNormalVertexList = new DrawRawVertex[TriangleContextList[i]->FaceList.size() * 2];
		memset(TriangleContextList[i]->VertexData->DrawFaceNormalVertexList, 0, TriangleContextList[i]->FaceList.size() * 2);

		if (TriangleContextList[i]->DrawFaceNormalIndexList != nullptr) delete[] TriangleContextList[i]->DrawFaceNormalIndexList;
		TriangleContextList[i]->DrawFaceNormalIndexList = new DrawRawIndex[TriangleContextList[i]->FaceList.size() * 2];
		memset(TriangleContextList[i]->DrawFaceNormalIndexList, 0, TriangleContextList[i]->FaceList.size() * 2);

		if (TriangleContextList[i]->VertexData->DrawVertexNormalVertexList != nullptr) delete[] TriangleContextList[i]->VertexData->DrawVertexNormalVertexList;
		TriangleContextList[i]->VertexData->DrawVertexNormalVertexList = new DrawRawVertex[TriangleContextList[i]->FaceList.size() * 3 * 2];
		memset(TriangleContextList[i]->VertexData->DrawVertexNormalVertexList, 0, TriangleContextList[i]->FaceList.size() * 3 * 2);

		if (TriangleContextList[i]->DrawVertexNormalIndexList != nullptr) delete[] TriangleContextList[i]->DrawVertexNormalIndexList;
		TriangleContextList[i]->DrawVertexNormalIndexList = new DrawRawIndex[TriangleContextList[i]->FaceList.size() * 3 * 2];
		memset(TriangleContextList[i]->DrawVertexNormalIndexList, 0, TriangleContextList[i]->FaceList.size() * 3 * 2);

		TriangleContextList[i]->VertexData->Bounding.Clear();

		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunFuncGenerateRenderData, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}


void* AdjacencyProcesser::RunFuncGenerateRenderData(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;
	VertexContext* VertexData = Src->VertexData;

	Face& CurrentFace= Src->FaceList[Src->CurrentFacePos];

	Vertex3& V1 = VertexData->VertexList[CurrentFace.x.value];
	Vertex3& V2 = VertexData->VertexList[CurrentFace.y.value];
	Vertex3& V3 = VertexData->VertexList[CurrentFace.z.value];

	Float3 Color = 0.0f; 
	float Alpha = 0.0f;

	Color.x = float(CurrentFace.meshletId[0]);
	if (Color.x == -1) std::cout << "Some Face Is Not Belong To Any Meshlet Layer 0" << std::endl;

	Color.y = float(CurrentFace.meshletId[1]);
	if (Color.y == -1) std::cout << "Some Face Is Not Belong To Any Meshlet Layer 1" << std::endl;

	Color.z = float(CurrentFace.meshletId[2]);
	if (Color.z == -1) std::cout << "Some Face Is Not Belong To Any Meshlet Layer 2" << std::endl;

	//Edge& E1 = Src->EdgeList[CurrentFace.xy];
	//if (E1.meshletId[0] == -1) std::cout << "Some Edge Is Not Belong To Any Meshlet" << std::endl;
	//Edge& E2 = Src->EdgeList[CurrentFace.yz];
	//if (E2.meshletId[0] == -1) std::cout << "Some Edge Is Not Belong To Any Meshlet" << std::endl;
	//Edge& E3 = Src->EdgeList[CurrentFace.zx];
	//if (E3.meshletId[0] == -1) std::cout << "Some Edge Is Not Belong To Any Meshlet" << std::endl;

	FaceNormal& NormalSet = Src->FaceNormalMap[Src->CurrentFacePos];
	DrawRawVertex NewVertex1(V1.Get(), NormalSet.normalvx, Color, Alpha);
	DrawRawVertex NewVertex2(V2.Get(), NormalSet.normalvy, Color, Alpha);
	DrawRawVertex NewVertex3(V3.Get(), NormalSet.normalvz, Color, Alpha);


	uint LineIndex[3];
	VertexData->DrawVertexList[Src->CurrentIndexPos] = NewVertex1;
	Src->DrawIndexList[Src->CurrentIndexPos] = Src->CurrentIndexPos;
	LineIndex[0] = Src->CurrentIndexPos;
	Src->CurrentIndexPos++;

	VertexData->DrawVertexList[Src->CurrentIndexPos] = NewVertex2;
	Src->DrawIndexList[Src->CurrentIndexPos] = Src->CurrentIndexPos;
	LineIndex[1] = Src->CurrentIndexPos;
	Src->CurrentIndexPos++;

	VertexData->DrawVertexList[Src->CurrentIndexPos] = NewVertex3;
	Src->DrawIndexList[Src->CurrentIndexPos] = Src->CurrentIndexPos;
	LineIndex[2] = Src->CurrentIndexPos;
	Src->CurrentIndexPos++;

	VertexData->Bounding.Resize(NewVertex1.pos);
	VertexData->Bounding.Resize(NewVertex2.pos);
	VertexData->Bounding.Resize(NewVertex3.pos);

	//face normal
	DrawRawVertex NewFaceNormalVertex1(CurrentFace.center, CurrentFace.normal, Color, Alpha);
	DrawRawVertex NewFaceNormalVertex2(CurrentFace.center + CurrentFace.normal * 0.1f, CurrentFace.normal, Color, Alpha);
	VertexData->DrawFaceNormalVertexList[2 * Src->CurrentFacePos] = NewFaceNormalVertex1;
	VertexData->DrawFaceNormalVertexList[2 * Src->CurrentFacePos + 1] = NewFaceNormalVertex2;
	Src->DrawFaceNormalIndexList[2 * Src->CurrentFacePos] = 2 * Src->CurrentFacePos;
	Src->DrawFaceNormalIndexList[2 * Src->CurrentFacePos + 1] = 2 * Src->CurrentFacePos + 1;

	//vertex normal
	DrawRawVertex NewVertexNormalV1(NewVertex1.pos + NewVertex1.normal * 0.1f, NewVertex1.normal, Color, Alpha);
	DrawRawVertex NewVertexNormalV2(NewVertex2.pos + NewVertex2.normal * 0.1f, NewVertex2.normal, Color, Alpha);
	DrawRawVertex NewVertexNormalV3(NewVertex3.pos + NewVertex3.normal * 0.1f, NewVertex3.normal, Color, Alpha);

	uint NormalVertexIndex = 3 * 2 * Src->CurrentFacePos;
	VertexData->DrawVertexNormalVertexList[NormalVertexIndex] = NewVertex1;
	VertexData->DrawVertexNormalVertexList[NormalVertexIndex + 1] = NewVertexNormalV1;
	Src->DrawVertexNormalIndexList[NormalVertexIndex] = NormalVertexIndex;
	Src->DrawVertexNormalIndexList[NormalVertexIndex + 1] = NormalVertexIndex + 1;

	VertexData->DrawVertexNormalVertexList[NormalVertexIndex + 2] = NewVertex2;
	VertexData->DrawVertexNormalVertexList[NormalVertexIndex + 3] = NewVertexNormalV2;
	Src->DrawVertexNormalIndexList[NormalVertexIndex + 2] = NormalVertexIndex + 2;
	Src->DrawVertexNormalIndexList[NormalVertexIndex + 3] = NormalVertexIndex + 3;

	VertexData->DrawVertexNormalVertexList[NormalVertexIndex + 4] = NewVertex3;
	VertexData->DrawVertexNormalVertexList[NormalVertexIndex + 5] = NewVertexNormalV3;
	Src->DrawVertexNormalIndexList[NormalVertexIndex + 4] = NormalVertexIndex + 4;
	Src->DrawVertexNormalIndexList[NormalVertexIndex + 5] = NormalVertexIndex + 5;


	Src->CurrentFacePos++;

	double Step = 1.0 / Src->FaceList.size();
	*OutProgressPerRun = Step;

	if (Src->CurrentFacePos >= Src->FaceList.size())
		return (void*)Src;
	else
		return nullptr;
}



void AdjacencyProcesser::Export(std::filesystem::path& FilePath)
{
	std::ofstream OutFile(FilePath.c_str(), std::ios::out | std::ios::binary);

	int MeshNum = TriangleContextList.size();

	assert(sizeof(float) == sizeof(uint));

	// layout
	size_t TotalBytesLength = sizeof(uint);// mesh num 
	TotalBytesLength += sizeof(float);// size of float
	TotalBytesLength += sizeof(uint);// size of uint
	for (int i = 0; i < MeshNum; i++)
	{
		TotalBytesLength += sizeof(uint);// name length
		TotalBytesLength += TriangleContextList[i]->Name.length(); // name

		// vertex data begin
		TotalBytesLength += sizeof(uint);// vertex num
		TotalBytesLength += sizeof(uint);// repeated vertex num
		TotalBytesLength += sizeof(uint);// struct size
		TotalBytesLength += TriangleContextList[i]->GetExportVertexDataByteSize();

		// face data begin
		TotalBytesLength += sizeof(uint);// face num
		TotalBytesLength += sizeof(uint);// struct size
		TotalBytesLength += TriangleContextList[i]->GetExportFaceDataByteSize();

		// edge data begin
		TotalBytesLength += sizeof(uint);// edge num
		TotalBytesLength += sizeof(uint);// struct size
		TotalBytesLength += 0;

		// meshlet data begin
		TotalBytesLength += sizeof(uint);// layer 0 meshlet num
		TotalBytesLength += sizeof(uint);// layer 1 meshlet num
		TotalBytesLength += sizeof(uint);// layer 2 meshlet num
		TotalBytesLength += sizeof(uint);// struct size
		TotalBytesLength += TriangleContextList[i]->GetExportMeshletDataByteSize();
	}


	// data
	size_t BytesOffset = 0;
	Byte* Buffer = new Byte[TotalBytesLength];
	memset(Buffer, 0, TotalBytesLength);

	WriteUnsignedIntegerToBytesLittleEndian(Buffer, &BytesOffset, MeshNum); 
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, &BytesOffset, sizeof(float)); 
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, &BytesOffset, sizeof(uint));

	
	for (int i = 0; i < MeshNum; i++)
	{
		uint NameLength = TriangleContextList[i]->Name.length();
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, &BytesOffset, NameLength);
		WriteASCIIStringToBytes(Buffer, &BytesOffset, TriangleContextList[i]->Name);


		WRITE_MESSAGE_DIGIT("Export Vertex List Num : ", TriangleContextList[i]->VertexData->VertexList.size());
		WRITE_MESSAGE_DIGIT("Export Repeated Vertex List Num : ", TriangleContextList[i]->EXPORTRepeatedVertexList.size());
		WRITE_MESSAGE_DIGIT("Export Repeated Vertex Struct Size : ", EXPORTVertex::ByteSize());
		TriangleContextList[i]->ExportVertexData(Buffer, &BytesOffset);
		WRITE_MESSAGE_DIGIT("Export Face List Num : ", TriangleContextList[i]->EXPORTFaceList.size());
		WRITE_MESSAGE_DIGIT("Export Face Struct Size : ", EXPORTFace::ByteSize());
		TriangleContextList[i]->ExportFaceData(Buffer, &BytesOffset);
		WRITE_MESSAGE_DIGIT("Export Edge Num : ", TriangleContextList[i]->EdgeList.size());
		WRITE_MESSAGE_DIGIT("Export Edge Struct Size : ", 0);
		TriangleContextList[i]->ExportEdgeData(Buffer, &BytesOffset);
		WRITE_MESSAGE_DIGIT("Export Meshlet List Num : ", TriangleContextList[i]->EXPORTMeshletList.size());
		WRITE_MESSAGE_DIGIT("Export Meshlet Layer1 Num : ", TriangleContextList[i]->MeshletLayer1Data.GetMeshletSize());
		WRITE_MESSAGE_DIGIT("Export Meshlet Layer2 Num : ", TriangleContextList[i]->MeshletLayer2Data.GetMeshletSize());
		WRITE_MESSAGE_DIGIT("Export Meshlet Struct Size : ", EXPORTMeshlet::ByteSize());
		TriangleContextList[i]->ExportMeshletData(Buffer, &BytesOffset);
	}

	OutFile.write((char*)Buffer, TotalBytesLength);
	OutFile.close();

	delete[] Buffer;
}

void SourceContext::ExportVertexData(Byte* Buffer, size_t* BytesOffset)
{
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, VertexData->VertexList.size());
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, EXPORTRepeatedVertexList.size());
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, EXPORTVertex::ByteSize());

	for (int j = 0; j < EXPORTRepeatedVertexList.size(); j++)
	{
		const EXPORTVertex& Curr = EXPORTRepeatedVertexList[j];
		WriteFloatToBytesLittleEndian(Buffer, BytesOffset, Curr.Position.x);
		WriteFloatToBytesLittleEndian(Buffer, BytesOffset, Curr.Position.y);
		WriteFloatToBytesLittleEndian(Buffer, BytesOffset, Curr.Position.z);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.UniqueIndex);

	}
}

void SourceContext::ExportFaceData(Byte* Buffer, size_t* BytesOffset)
{
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, EXPORTFaceList.size());
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, EXPORTFace::ByteSize());

	for (int j = 0; j < EXPORTFaceList.size(); j++)
	{
		const EXPORTFace& Curr = EXPORTFaceList[j];
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.v012);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.edge01);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.edge12);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.edge20);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.UniqueIndex01);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.UniqueIndex12);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.UniqueIndex20);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.PackNormalData0.x);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.PackNormalData0.y);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.PackNormalData0.z);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.PackNormalData1.x);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.PackNormalData1.y);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.PackNormalData1.z);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.MeshletData[0]);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.MeshletData[1]);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.MeshletData[2]);
	}
}

void SourceContext::ExportEdgeData(Byte* Buffer, size_t* BytesOffset)
{
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, EdgeList.size());
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, 0);
}

void SourceContext::ExportMeshletData(Byte* Buffer, size_t* BytesOffset)
{
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, EXPORTMeshletList.size());
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, MeshletLayer1Data.GetMeshletSize());
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, MeshletLayer2Data.GetMeshletSize());
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, EXPORTMeshlet::ByteSize());

	for (int j = 0; j < EXPORTMeshletList.size(); j++)
	{
		const EXPORTMeshlet& Curr = EXPORTMeshletList[j];
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.FaceOffset);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.FaceNum);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.VertexOffset);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.VertexNum);
	}
}




bool PassGenerateVertexMap(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::filesystem::path VertexFilePath = FilePath;
	VertexFilePath.replace_extension(std::filesystem::path("vertices"));
	Success = Processer->GetReady0(VertexFilePath);


	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Generate Vertex Map....... ";

	return Success;


	/*
	std::vector<VertexContext*> ContextList = Processer.GetVertexContextList();
	VertexContext* Context = ContextList[0];

	ContextList[0]->DumpIndexMap();
	ContextList[0]->DumpVertexList();
	ContextList[0]->DumpRawVertexList();
	*/


}


bool PassGenerateFaceAndEdgeData(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::filesystem::path TriangleFilePath = FilePath;
	Success = Processer->GetReady1(TriangleFilePath);
	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Generate Face And Edge Data....... ";

	return Success;

}



bool PassGenerateVertexNormal(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::filesystem::path TriangleFilePath = FilePath;
	Success = Processer->GetReady2();
	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Generate Vertex Normal....... ";

	return Success;

}


bool PassGenerateMeshletLayer01Data(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::cout << LINE_STRING << std::endl;
	std::cout << "Begin Generating Meshlet Layer 0&1 Data..." << std::endl;
	Success = Processer->GetReadyForGenerateMeshletLayer01Data();

	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Generating Meshlet Layer 0&1 Data....... ";

	return Success;
}


bool PassGenerateMeshletLayer0(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::filesystem::path TriangleFilePath = FilePath;
	Success = Processer->GetReadyGenerateMeshletLayer0();
	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Generate Meshlet Layer 0....... ";

	return Success;

}


bool PassSerializeMeshLayer0Data(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::filesystem::path TriangleFilePath = FilePath;
	Success = Processer->GetReadyForSerializeMeshletLayer0();
	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Serializing Meshlet Layer 0....... ";

	return Success;

}

bool PassGenerateMeshletLayer1(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::filesystem::path TriangleFilePath = FilePath;
	Success = Processer->GetReadyGenerateMeshletLayer1();
	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Generate Meshlet Layer 1....... ";

	return Success;

}


bool PassSerializeMeshLayer1Data(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::filesystem::path TriangleFilePath = FilePath;
	Success = Processer->GetReadyForSerializeMeshletLayer1();
	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Serializing Meshlet Layer 1....... ";

	return Success;

}



bool PassGenerateMeshletLayer2Data(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::cout << LINE_STRING << std::endl;
	std::cout << "Begin Generating Meshlet Layer 2 Data..." << std::endl;
	Success = Processer->GetReadyForGenerateMeshletLayer2Data();

	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Generating Meshlet Layer 2 Data....... ";

	return Success;
}


bool PassGenerateMeshletLayer2(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::filesystem::path TriangleFilePath = FilePath;
	Success = Processer->GetReadyGenerateMeshletLayer2();
	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Generate Meshlet....... ";

	return Success;

}


bool PassSerializeMeshLayer2Data(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::filesystem::path TriangleFilePath = FilePath;
	Success = Processer->GetReadyForSerializeMeshletLayer2();
	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Serializing Meshlet Layer 2....... ";

	return Success;

}


bool PassSerializeExportData(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::filesystem::path TriangleFilePath = FilePath;
	Success = Processer->GetReadyForSerializeExportData();
	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Serializing Data For Exporting....... ";

	return Success;

}

bool PassSerializeExportData2(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::filesystem::path TriangleFilePath = FilePath;
	Success = Processer->GetReadyForSerializeExportData2();
	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Serializing Data For Exporting....... ";

	return Success;

}



bool PassGenerateRenderData(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::cout << LINE_STRING << std::endl;
	std::cout << "Begin Generating Render Data..." << std::endl;
	Success = Processer->GetReadyGenerateRenderData();

	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Generating Render Data....... ";

	return Success;

}


void Export(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	std::cout << LINE_STRING << std::endl;
	std::cout << "Exporting......" << std::endl;

	Processer->ClearMessageString();
	std::filesystem::path ExportFilePath = FilePath;
	ExportFilePath.replace_extension(std::filesystem::path("linemeta"));

	Processer->Export(ExportFilePath);

	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	//        //DebugCheck
//std::vector<SourceContext*> ContextList = Processer->GetTriangleContextList();
//SourceContext* Context = ContextList[0];
//ContextList[0]->DumpEdgeList();
//ContextList[0]->DumpFaceList();
//        //ContextList[0]->DumpEdgeFaceList();
//        //ContextList[0]->DumpAdjacencyFaceListShrink();
//        //ContextList[0]->DumpAdjacencyVertexList();
//std::vector<VertexContext*> VContextList = Processer->GetVertexContextList();
//        //VContextList[0]->DumpIndexMap();
//VContextList[0]->DumpVertexList();

	State = "Export Completed.";
}


