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
		Context->RawVertex.reserve(vertexLength);

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

	float x = BytesToFloatLittleEndian(Src->BytesData, Src->CurrentVertexPos); Src->CurrentVertexPos += VER_ELEMENT_LENGTH;
	float y = BytesToFloatLittleEndian(Src->BytesData, Src->CurrentVertexPos); Src->CurrentVertexPos += VER_ELEMENT_LENGTH;
	float z = BytesToFloatLittleEndian(Src->BytesData, Src->CurrentVertexPos); Src->CurrentVertexPos += VER_ELEMENT_LENGTH;

	Vertex3 v = Vertex3(x, y, z);
	uint VertexId = Src->CurrentVertexId;

	Src->RawVertex.emplace_back(v);

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


bool AdjacencyProcesser::GetReady1(std::filesystem::path& FilePath, bool NeedMergeDuplicateVertex)
{
	if (AsyncProcesser == nullptr)
	{
		AsyncProcesser = new ThreadProcesser();
	}
	else
	{
		if(!NeedMergeDuplicateVertex)
			Clear();
		else
		{
			AsyncProcesser->Clear();
			ErrorString = "";
			MessageString = "";
		}
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
		if (!NeedMergeDuplicateVertex)
		{
			VertexContext* VertexData = new VertexContext();
			VertexContextList.push_back(VertexData);
			Context->VertexData = VertexData;
		}
		else
			Context->VertexData = VertexContextList[i];

		Context->FaceList.reserve(Context->TriangleNum);

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

	Edge e1 = Edge(ix, iy);
	Edge e2 = Edge(iy, iz);
	Edge e3 = Edge(iz, ix);

	FacePair adj = FacePair();
	adj.face1 = FaceId;
	adj.set1 = true;

	std::unordered_map<Edge, FacePair>::iterator it1;
	it1 = Src->EdgeList.find(e1);
	if (it1 != Src->EdgeList.end())
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

#if DEBUG_2
		ErrorString += "Write Edge: Found " + std::to_string(e1.v1.actual_value) + ", " + std::to_string(e1.v2.actual_value) + " | " + std::to_string(it1->second.face1) + ", " + std::to_string(it1->second.face2) + "\n";
#endif
	}
	else
	{
		Src->EdgeList.insert(std::unordered_map<Edge, FacePair>::value_type(e1, adj));
#if DEBUG_2
		ErrorString += "Write Edge: NOTFound " + std::to_string(e1.v1.actual_value) + ", " + std::to_string(e1.v2.actual_value) + " | " + std::to_string(adj.face1) + ", " + std::to_string(adj.face2) + "\n";
#endif
	}

	std::unordered_map<Edge, FacePair>::iterator it2 = Src->EdgeList.find(e2);
	if (it2 != Src->EdgeList.end())
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
#if DEBUG_2
		ErrorString += "Write Edge: Found " + std::to_string(e2.v1.actual_value) + ", " + std::to_string(e2.v2.actual_value) + " | " + std::to_string(it2->second.face1) + ", " + std::to_string(it2->second.face2) + "\n";
#endif
	}
	else
	{
		Src->EdgeList.insert(std::unordered_map<Edge, FacePair>::value_type(e2, adj));
#if DEBUG_2
		ErrorString += "Write Edge: NOTFound " + std::to_string(e2.v1.actual_value) + ", " + std::to_string(e2.v2.actual_value) + " | " + std::to_string(adj.face1) + ", " + std::to_string(adj.face2) + "\n";
#endif
	}

	std::unordered_map<Edge, FacePair>::iterator it3 = Src->EdgeList.find(e3);
	if (it3 != Src->EdgeList.end())
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
#if DEBUG_2
		ErrorString += "Write Edge: Found " + std::to_string(e3.v1.actual_value) + ", " + std::to_string(e3.v2.actual_value) + " | " + std::to_string(it3->second.face1) + ", " + std::to_string(it3->second.face2) + "\n";
#endif
	}
	else
	{
		Src->EdgeList.insert(std::unordered_map<Edge, FacePair>::value_type(e3, adj));
#if DEBUG_2
		ErrorString += "Write Edge: NOTFound " + std::to_string(e3.v1.actual_value) + ", " + std::to_string(e3.v2.actual_value) + " | " + std::to_string(adj.face1) + ", " + std::to_string(adj.face2) + "\n";
#endif
	}

	Src->FaceList.emplace_back(Face(ix, iy, iz));



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

		if (TriangleContextList[i]->FaceList.size() >= 1) {
			TriangleContextList[i]->FaceList[0].BlackWhite = 1;
			TriangleContextList[i]->FaceIdPool.push(0);

			TriangleContextList[i]->AdjacencyFaceList.reserve(TriangleContextList[i]->FaceList.size());
		}

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


bool AdjacencyProcesser::HandleAdjacencyFace(
	const uint CurrentFaceId,
	const Edge& EdgeToSearch,
	uint EdgeIndex, 
	SourceContext* Src,
	uint* OutAdjFaceId, AdjFace* OutAdjFace)
{
	bool HasAdjFace = false;
	std::unordered_map<Edge, FacePair>::iterator it = Src->EdgeList.find(EdgeToSearch);
	if (it != Src->EdgeList.end())
	{
		FacePair& adj = it->second;
		if (adj.set1 && adj.face1 != CurrentFaceId)
		{
			OutAdjFace->adjPoint[EdgeIndex] = Src->FaceList[adj.face1].GetOppositePoint(EdgeToSearch.v1, EdgeToSearch.v2);
			OutAdjFace->hasAdjFace[EdgeIndex] = true;
			*OutAdjFaceId = adj.face1;
			HasAdjFace = true;
		}
		else if (adj.set2 && adj.face2 != CurrentFaceId)
		{
			OutAdjFace->adjPoint[EdgeIndex] = Src->FaceList[adj.face2].GetOppositePoint(EdgeToSearch.v1, EdgeToSearch.v2);
			OutAdjFace->hasAdjFace[EdgeIndex] = true;
			*OutAdjFaceId = adj.face2;
			HasAdjFace = true;
		}
	}
#if DEBUG_3
	else
	{
		ErrorString += "Find Edge Error. Current Face: " + std::to_string(CurrentFaceId) + " | " + std::to_string(OutAdjFace->x.actual_value) + "," + std::to_string(OutAdjFace->y.actual_value) + "," + std::to_string(OutAdjFace->z.actual_value) + "\n";
		ErrorString += "Edge: " + std::to_string(EdgeToSearch.v1.actual_value) + ", " + std::to_string(EdgeToSearch.v2.actual_value) + "\n";
	}
#endif

	return HasAdjFace;
}


bool AdjacencyProcesser::QueryAdjacencyFace(
	const uint CurrentFaceId,
	const Edge& EdgeToSearch,
	uint EdgeIndex,
	SourceContext* Src,
	uint* OutAdjFaceId)
{
	bool HasAdjFace = false;
	std::unordered_map<Edge, FacePair>::iterator it = Src->EdgeList.find(EdgeToSearch);
	if (it != Src->EdgeList.end())
	{
		FacePair& adj = it->second;
		if (adj.set1 && adj.face1 != CurrentFaceId)
		{
			*OutAdjFaceId = adj.face1;
			HasAdjFace = true;
		}
		else if (adj.set2 && adj.face2 != CurrentFaceId)
		{
			*OutAdjFaceId = adj.face2;
			HasAdjFace = true;
		}
	}
#if DEBUG_3
	else
	{
		ErrorString += "Find Edge Error. Current Face: " + std::to_string(CurrentFaceId) + "\n";
		ErrorString += "Edge: " + std::to_string(EdgeToSearch.v1.actual_value) + ", " + std::to_string(EdgeToSearch.v2.actual_value) + "\n";
	}
#endif

	return HasAdjFace;
}

void* AdjacencyProcesser::RunFunc2(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	if (!Src->FaceIdPool.empty()) {
		int CurrentFaceId = Src->FaceIdPool.front();

		Face& CurrentFace = Src->FaceList[CurrentFaceId];

		
		if (!CurrentFace.IsRead)
		{
			Edge xy(CurrentFace.x, CurrentFace.y);
			Edge yz(CurrentFace.y, CurrentFace.z);
			Edge zx(CurrentFace.z, CurrentFace.x);

			uint xy_adj_face_id = 0;
			uint yz_adj_face_id = 0;
			uint zx_adj_face_id = 0;

			if (CurrentFace.BlackWhite == 1)
			{
				AdjFace NewAdjFace(CurrentFace);

				bool xy_has_adj_face = HandleAdjacencyFace(CurrentFaceId, xy, 0, Src, &xy_adj_face_id, &NewAdjFace);
				bool yz_has_adj_face = HandleAdjacencyFace(CurrentFaceId, yz, 1, Src, &yz_adj_face_id, &NewAdjFace);
				bool zx_has_adj_face = HandleAdjacencyFace(CurrentFaceId, zx, 2, Src, &zx_adj_face_id, &NewAdjFace);

				Src->AdjacencyFaceList.emplace_back(NewAdjFace);

				if (xy_has_adj_face && !Src->FaceList[xy_adj_face_id].IsRead)
				{
					if (Src->FaceList[xy_adj_face_id].BlackWhite == 0)
					{
						Src->FaceList[xy_adj_face_id].BlackWhite = -1;
						Src->FaceIdPool.push(xy_adj_face_id);
					}
				}
				if (yz_has_adj_face && !Src->FaceList[yz_adj_face_id].IsRead)
				{
					if (Src->FaceList[yz_adj_face_id].BlackWhite == 0)
					{
						Src->FaceList[yz_adj_face_id].BlackWhite = -1;
						Src->FaceIdPool.push(yz_adj_face_id);
					}
				}
				if (zx_has_adj_face && !Src->FaceList[zx_adj_face_id].IsRead)
				{
					if (Src->FaceList[zx_adj_face_id].BlackWhite == 0)
					{
						Src->FaceList[zx_adj_face_id].BlackWhite = -1;
						Src->FaceIdPool.push(zx_adj_face_id);
					}
				}
				CurrentFace.IsRead = true;
				
			}
			else if (CurrentFace.BlackWhite == -1)
			{
				bool NeedRevert = false;

				bool xy_has_adj_face = QueryAdjacencyFace(CurrentFaceId, xy, 0, Src, &xy_adj_face_id);
				bool yz_has_adj_face = QueryAdjacencyFace(CurrentFaceId, yz, 1, Src, &yz_adj_face_id);
				bool zx_has_adj_face = QueryAdjacencyFace(CurrentFaceId, zx, 2, Src, &zx_adj_face_id);

				if (xy_has_adj_face && !Src->FaceList[xy_adj_face_id].IsRead)
				{
					if (Src->FaceList[xy_adj_face_id].BlackWhite == 0)
					{
						Src->FaceList[xy_adj_face_id].BlackWhite = 1;
						Src->FaceIdPool.push(xy_adj_face_id);
					}
					else if (Src->FaceList[xy_adj_face_id].BlackWhite == -1)
					{
						NeedRevert = true;
					}
				}
				if (yz_has_adj_face && !Src->FaceList[yz_adj_face_id].IsRead)
				{
					if (Src->FaceList[yz_adj_face_id].BlackWhite == 0)
					{
						Src->FaceList[yz_adj_face_id].BlackWhite = 1;
						Src->FaceIdPool.push(yz_adj_face_id);
					}
					else if (Src->FaceList[yz_adj_face_id].BlackWhite == -1)
					{
						NeedRevert = true;
					}
				}
				if (zx_has_adj_face && !Src->FaceList[zx_adj_face_id].IsRead)
				{
					if (Src->FaceList[zx_adj_face_id].BlackWhite == 0)
					{
						Src->FaceList[zx_adj_face_id].BlackWhite = 1;
						Src->FaceIdPool.push(zx_adj_face_id);
					}
					else if (Src->FaceList[zx_adj_face_id].BlackWhite == -1)
					{
						NeedRevert = true;
					}
				}

				if (NeedRevert)
				{
					CurrentFace.BlackWhite = 1;
				}
				else
					CurrentFace.IsRead = true;
			}	
		}

		double Step = 0.0;
		if (CurrentFace.IsRead) {
			Src->FaceIdPool.pop();
			Step = 1.0 / Src->TriangleNum;
		}
		*OutProgressPerRun = Step;
	}
	else {
		*OutProgressPerRun = 1.0;
	}



	if (Src->FaceIdPool.empty())
		return (void*)Src;
	else
		return nullptr;
}