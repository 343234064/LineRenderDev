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

	Src->FaceList.emplace_back(Face(ix, iy, iz, e1.id, e2.id, e3.id, Normal, Center));


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

void* AdjacencyProcesser::RunFunc2(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;
	VertexContext* VertexData = Src->VertexData;

	Vertex3& CurrentVertex = VertexData->VertexList[Src->CurrentIndexPos];
	
	Float3 Normal;

	std::set<uint>& AdjacencyFaceList = Src->AdjacencyVertexFaceMap[Src->CurrentIndexPos];
	if (AdjacencyFaceList.size() > 0) {

		std::vector<Float3> FaceNormals;
		for (std::set<uint>::iterator i = AdjacencyFaceList.begin(); i != AdjacencyFaceList.end(); i++)
		{
			Float3 CurrentFaceNormal = Src->FaceList[*i].normal;
			bool Add = true;
			for (uint j = 0; j < FaceNormals.size(); j++)
			{
				if (Dot(FaceNormals[j], CurrentFaceNormal) >= 0.99988f)
				{
					Add = false;
					break;
				}
			}
			if (Add)
				FaceNormals.push_back(CurrentFaceNormal);

		}
		for (std::vector<Float3>::iterator j = FaceNormals.begin(); j != FaceNormals.end(); j++)
		{
			Normal = Normal + *j;
		}

		Normal = Normalize(Normal);
	}
	else
		Normal = Normalize(CurrentVertex.Get());


	CurrentVertex.normal = Normal;


	Src->CurrentIndexPos++;

	double Step = 1.0 / VertexData->VertexList.size();
	*OutProgressPerRun = Step;

	if (Src->CurrentIndexPos >= VertexData->VertexList.size())
		return (void*)Src;
	else
		return nullptr;
}



bool AdjacencyProcesser::GetReadyForGenerateMeshletLayer1Data()
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

		TriangleContextList[i]->MeshletLayer1Data.Init(TriangleContextList[i]->VertexData->VertexList.size(), TriangleContextList[i]->FaceList.size());
		

		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunGenerateMeshletLayer1Data, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}



void* AdjacencyProcesser::RunGenerateMeshletLayer1Data(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;
	VertexContext* VertexData = Src->VertexData;
	MeshOpt& CurrentMeshletData = Src->MeshletLayer1Data;

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

	MeshOptTriangle NewCell = MeshOptTriangle(CurrentFace.center, CurrentFace.normal, Area, CurrentFace.x.value, CurrentFace.y.value, CurrentFace.z.value);

	//find adjacency triangle
	std::unordered_map<uint, std::set<uint>>::iterator it = Src->AdjacencyVertexFaceMap.find(CurrentFace.x.value);
	if (it != Src->AdjacencyVertexFaceMap.end())
	{
		std::set<uint>& AdjFaceList = it->second;
		for (std::set<uint>::iterator i = AdjFaceList.begin(); i != AdjFaceList.end(); i++)
		{
			CurrentMeshletData.vertices[CurrentFace.x.value].neighborTriangles.insert(*i);
		}
		CurrentMeshletData.live_triangles[CurrentFace.x.value] = AdjFaceList.size();
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
			CurrentMeshletData.vertices[CurrentFace.y.value].neighborTriangles.insert(*i);
		}
		CurrentMeshletData.live_triangles[CurrentFace.y.value] = AdjFaceList.size();;
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
			CurrentMeshletData.vertices[CurrentFace.z.value].neighborTriangles.insert(*i);
		}
		CurrentMeshletData.live_triangles[CurrentFace.z.value] = AdjFaceList.size();
	}
	else
	{
		ErrorString += "Cannot find vertex in AdjacencyVertexFaceMap. Current Vertex: " + std::to_string(CurrentFace.z.value) + "\n";
	}

	CurrentMeshletData.mesh_area = CurrentMeshletData.mesh_area + NewCell.Area;
	CurrentMeshletData.triangles.push_back(NewCell);
	CurrentMeshletData.kdindices[Src->CurrentFacePos] = unsigned int(Src->CurrentFacePos);

	Src->CurrentFacePos++;

	double Step = 1.0 / Src->FaceList.size();
	*OutProgressPerRun = Step;

	if (Src->CurrentFacePos >= Src->FaceList.size())
		return (void*)Src;
	else
		return nullptr;
}


bool AdjacencyProcesser::GetReadyGenerateMeshlet()
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

		size_t MaxTrianglesNumInMeshlet = 256;
		TriangleContextList[i]->MeshletLayer1Data.GetReady(MaxTrianglesNumInMeshlet, TriangleContextList[i]->FaceList.size(), TriangleContextList[i]->VertexData->VertexList.size(), MeshletNormalWeight);

		WRITE_MESSAGE_DIGIT("Max Cell Num In Meshlet: ", MaxTrianglesNumInMeshlet);
		WRITE_MESSAGE_DIGIT("Current Cell Num: ", TriangleContextList[i]->MeshletLayer1Data.triangles.size());
		WRITE_MESSAGE_DIGIT("ExpectedRadius: ", TriangleContextList[i]->MeshletLayer1Data.meshlet_expected_radius);

		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunGenerateMeshlet, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}

void* AdjacencyProcesser::RunGenerateMeshlet(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	if (Src->MeshletLayer1Data.RunStep(OutProgressPerRun)) {
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


	double Step = 0.0;

	const MeshOpt& MeshletLayer1Data = Src->MeshletLayer1Data;
	if (MeshletLayer1Data.GetMeshletSize() > Src->CurrentIndexPos) {
		const meshopt_Meshlet& CurrentMeshlet = MeshletLayer1Data.meshlets_list[Src->CurrentIndexPos];

		if (CurrentMeshlet.triangle_count > Src->CurrentFacePos) {
			unsigned int  CurrentFaceIndex = MeshletLayer1Data.meshlet_triangles[CurrentMeshlet.triangle_offset + Src->CurrentFacePos]; 

			Face& DestFace = Src->FaceList[CurrentFaceIndex];
			DestFace.meshletId[0] = Src->CurrentIndexPos;

			Step = 0.0;
			Src->CurrentFacePos++;
		}
		else
		{
			//if (CurrentMeshlet.triangle_count < 256)
			//	std::cout << Src->Name << "|" <<CurrentMeshlet.triangle_count << std::endl;
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
		TriangleContextList[i]->VertexData->DrawVertexNormalVertexList = new DrawRawVertex[TriangleContextList[i]->VertexData->VertexList.size() * 2];
		memset(TriangleContextList[i]->VertexData->DrawVertexNormalVertexList, 0, TriangleContextList[i]->VertexData->VertexList.size() * 2);

		if (TriangleContextList[i]->DrawVertexNormalIndexList != nullptr) delete[] TriangleContextList[i]->DrawVertexNormalIndexList;
		TriangleContextList[i]->DrawVertexNormalIndexList = new DrawRawIndex[TriangleContextList[i]->VertexData->VertexList.size() * 2];
		memset(TriangleContextList[i]->DrawVertexNormalIndexList, 0, TriangleContextList[i]->VertexData->VertexList.size() * 2);

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
	if (Color.x == -1) std::cout << "Some Face Is Not Belong To Any Meshlet" << std::endl;


	DrawRawVertex NewVertex1(V1.Get(), V1.normal, Color, Alpha);
	DrawRawVertex NewVertex2(V2.Get(), V2.normal, Color, Alpha);
	DrawRawVertex NewVertex3(V3.Get(), V3.normal, Color, Alpha);


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
	DrawRawVertex NewVertexNormalV1(V1.Get() + V1.normal * 0.1f, V1.normal, Color, Alpha);
	DrawRawVertex NewVertexNormalV2(V2.Get() + V2.normal * 0.1f, V2.normal, Color, Alpha);
	DrawRawVertex NewVertexNormalV3(V3.Get() + V3.normal * 0.1f, V3.normal, Color, Alpha);

	uint V1Index = CurrentFace.x.value;
	VertexData->DrawVertexNormalVertexList[2 * V1Index] = NewVertex1;
	VertexData->DrawVertexNormalVertexList[2 * V1Index + 1] = NewVertexNormalV1;
	Src->DrawVertexNormalIndexList[2 * V1Index] = 2 * V1Index;
	Src->DrawVertexNormalIndexList[2 * V1Index + 1] = 2 * V1Index + 1;

	uint V2Index = CurrentFace.y.value;
	VertexData->DrawVertexNormalVertexList[2 * V2Index] = NewVertex2;
	VertexData->DrawVertexNormalVertexList[2 * V2Index + 1] = NewVertexNormalV2;
	Src->DrawVertexNormalIndexList[2 * V2Index] = 2 * V2Index;
	Src->DrawVertexNormalIndexList[2 * V2Index + 1] = 2 * V2Index + 1;

	uint V3Index = CurrentFace.z.value;
	VertexData->DrawVertexNormalVertexList[2 * V3Index] = NewVertex3;
	VertexData->DrawVertexNormalVertexList[2 * V3Index + 1] = NewVertexNormalV3;
	Src->DrawVertexNormalIndexList[2 * V3Index] = 2 * V3Index;
	Src->DrawVertexNormalIndexList[2 * V3Index + 1] = 2 * V3Index + 1;


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

	uint UintBytesSize = sizeof(uint);

	// layout
	uint TotalBytesLength = UintBytesSize;// mesh num 
	TotalBytesLength += UintBytesSize;// size of float
	TotalBytesLength += UintBytesSize;// size of uint
	for (int i = 0; i < MeshNum; i++)
	{
		TotalBytesLength += UintBytesSize;// name length
		TotalBytesLength += TriangleContextList[i]->Name.length(); // name

		// vertex data begin
		TotalBytesLength += UintBytesSize;// vertex num
		TotalBytesLength += UintBytesSize;// per struct size
		TotalBytesLength += TriangleContextList[i]->GetVertexDataByteSize();


	}

	uint BytesOffset = 0;
	Byte* Buffer = new Byte[TotalBytesLength];
	memset(Buffer, 0, TotalBytesLength);

	WriteUnsignedIntegerToBytesLittleEndian(Buffer, &BytesOffset, MeshNum); 
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, &BytesOffset, sizeof(float)); 
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, &BytesOffset, UintBytesSize);

	
	for (int i = 0; i < MeshNum; i++)
	{
		uint NameLength = TriangleContextList[i]->Name.length();
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, &BytesOffset, NameLength);
		WriteASCIIStringToBytes(Buffer, &BytesOffset, TriangleContextList[i]->Name);

		ExportVertexData(Buffer, &BytesOffset, TriangleContextList[i]);

	}

	OutFile.write((char*)Buffer, TotalBytesLength);
	OutFile.close();

	delete[] Buffer;
}

void AdjacencyProcesser::ExportVertexData(Byte* Buffer, uint* BytesOffset, const SourceContext* Context)
{
	const VertexContext* VContext = Context->VertexData;

	WRITE_MESSAGE_DIGIT("Vertex Num : ", VContext->VertexList.size());
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, VContext->VertexList.size());
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Vertex3::ByteSize());

	for (int j = 0; j < VContext->VertexList.size(); j++)
	{
		const Vertex3& Curr = VContext->VertexList[j];
		WriteFloatToBytesLittleEndian(Buffer, BytesOffset, Curr.x);
		WriteFloatToBytesLittleEndian(Buffer, BytesOffset, Curr.y);
		WriteFloatToBytesLittleEndian(Buffer, BytesOffset, Curr.z);
		//WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.adjSerializedId);
		//WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.adjNum);

		////is vertex meshlet border
		//WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.IsMeshletBorder[0] ? 1 : 0);
		//WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.IsMeshletBorder[1] ? 1 : 0);
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

bool PassGenerateMeshletLayer1Data(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::cout << LINE_STRING << std::endl;
	std::cout << "Begin Generating Meshlet Layer 1 Data..." << std::endl;
	Success = Processer->GetReadyForGenerateMeshletLayer1Data();

	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Generating Meshlet Layer 1 Data....... ";

	return Success;
}


bool PassGenerateMeshlet(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)
{
	bool Success = false;

	std::filesystem::path TriangleFilePath = FilePath;
	Success = Processer->GetReadyGenerateMeshlet();
	std::cout << LINE_STRING << std::endl;
	std::cout << Processer->GetMessageString() << std::endl;
	std::cout << LINE_STRING << std::endl;

	if (Success)
		State = "Generate Meshlet....... ";

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
		State = "Serializing Meshlet Layer1....... ";

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


