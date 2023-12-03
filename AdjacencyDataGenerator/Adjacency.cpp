#include "Adjacency.h"
#include <iostream>


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


inline int BytesToUnsignedIntegerLittleEndian(Byte* Src, uint Offset)
{
	return static_cast<int>(static_cast<Byte>(Src[Offset]) |
		static_cast<Byte>(Src[Offset + 1]) << 8 |
		static_cast<Byte>(Src[Offset + 2]) << 16 |
		static_cast<Byte>(Src[Offset + 3]) << 24);
}

inline float BytesToFloatLittleEndian(Byte* Src, uint Offset)
{
	uint32_t value = static_cast<uint32_t>(Src[Offset]) |
		static_cast<uint32_t>(Src[Offset + 1]) << 8 |
		static_cast<uint32_t>(Src[Offset + 2]) << 16 |
		static_cast<uint32_t>(Src[Offset + 3]) << 24;

	return *reinterpret_cast<float*>(&value);
}
 
inline std::string BytesToASCIIString(Byte* Src, int Offset, int Length)
{
	char* Buffer = new char[size_t(Length) + 1];
	memcpy(Buffer, Src + Offset, Length);
	Buffer[Length] = '\0';

	std::string Str = Buffer;
	delete[] Buffer;
	return std::move(Str);
}


inline void WriteUnsignedIntegerToBytesLittleEndian(Byte* Src, uint* Offset, uint Value)
{
	Src[*Offset] = Value & 0x000000ff;
	Src[*Offset + 1] = (Value & 0x0000ff00) >> 8;
	Src[*Offset + 2] = (Value & 0x00ff0000) >> 16;
	Src[*Offset + 3] = (Value & 0xff000000) >> 24;
	*Offset += 4;
}

inline void WriteASCIIStringToBytes(Byte* Dst, uint* Offset, std::string& Value)
{
	memcpy(Dst + *Offset, Value.c_str(), Value.length());
	*Offset += Value.length();
}

inline void WriteFloatToBytesLittleEndian(Byte* Src, uint* Offset, float Value)
{
	uint* T = (uint*)&Value;
	uint V = *T;
	WriteUnsignedIntegerToBytesLittleEndian(Src, Offset, V);
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

	Src->FaceList.emplace_back(Face(ix, iy, iz, e1.id, e2.id, e3.id));
	Src->FaceIdPool.insert(Src->CurrentFacePos);


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
		WRITE_MESSAGE_DIGIT("Edge: ", TriangleContextList[i]->EdgeFaceList.size());
	}


	for (uint i = 0; i < TriangleContextList.size() ; i++)
	{	
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;

		if (TriangleContextList[i]->FaceList.size() >= 1) {
			TriangleContextList[i]->FaceList[0].BlackWhite = 1;
			TriangleContextList[i]->FaceIdQueue.push(0);
			TriangleContextList[i]->FaceIdPool.erase(0);

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
	std::unordered_map<Edge, FacePair>::iterator it = Src->EdgeFaceList.find(EdgeToSearch);
	if (it != Src->EdgeFaceList.end())
	{
		FacePair& adj = it->second;
		if (adj.set1 && adj.face1 != CurrentFaceId)
		{
			OutAdjFace->adjPoint[EdgeIndex] = Src->FaceList[adj.face1].GetOppositePoint(EdgeToSearch.v1, EdgeToSearch.v2);
			OutAdjFace->hasAdjFace[EdgeIndex] = true;
			OutAdjFace->adjFaceIndex[EdgeIndex] = adj.face1;
			*OutAdjFaceId = adj.face1;
			HasAdjFace = true;
		}
		else if (adj.set2 && adj.face2 != CurrentFaceId)
		{
			OutAdjFace->adjPoint[EdgeIndex] = Src->FaceList[adj.face2].GetOppositePoint(EdgeToSearch.v1, EdgeToSearch.v2);
			OutAdjFace->hasAdjFace[EdgeIndex] = true;
			OutAdjFace->adjFaceIndex[EdgeIndex] = adj.face2;
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
	std::unordered_map<Edge, FacePair>::iterator it = Src->EdgeFaceList.find(EdgeToSearch);
	if (it != Src->EdgeFaceList.end())
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

	if (!Src->FaceIdQueue.empty()) {
		int CurrentFaceId = Src->FaceIdQueue.front();

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
				NewAdjFace.faceIndex = CurrentFaceId;

				bool xy_has_adj_face = HandleAdjacencyFace(CurrentFaceId, xy, 0, Src, &xy_adj_face_id, &NewAdjFace);
				bool yz_has_adj_face = HandleAdjacencyFace(CurrentFaceId, yz, 1, Src, &yz_adj_face_id, &NewAdjFace);
				bool zx_has_adj_face = HandleAdjacencyFace(CurrentFaceId, zx, 2, Src, &zx_adj_face_id, &NewAdjFace);

				Src->AdjacencyFaceList.emplace_back(NewAdjFace);

				if (xy_has_adj_face && !Src->FaceList[xy_adj_face_id].IsRead)
				{
					if (Src->FaceList[xy_adj_face_id].BlackWhite == 0)
					{
						Src->FaceList[xy_adj_face_id].BlackWhite = -1;
						Src->FaceIdQueue.push(xy_adj_face_id);
						Src->FaceIdPool.erase(xy_adj_face_id);
					}
				}
				if (yz_has_adj_face && !Src->FaceList[yz_adj_face_id].IsRead)
				{
					if (Src->FaceList[yz_adj_face_id].BlackWhite == 0)
					{
						Src->FaceList[yz_adj_face_id].BlackWhite = -1;
						Src->FaceIdQueue.push(yz_adj_face_id);
						Src->FaceIdPool.erase(yz_adj_face_id);
					}
				}
				if (zx_has_adj_face && !Src->FaceList[zx_adj_face_id].IsRead)
				{
					if (Src->FaceList[zx_adj_face_id].BlackWhite == 0)
					{
						Src->FaceList[zx_adj_face_id].BlackWhite = -1;
						Src->FaceIdQueue.push(zx_adj_face_id);
						Src->FaceIdPool.erase(zx_adj_face_id);
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
						Src->FaceIdQueue.push(xy_adj_face_id);
						Src->FaceIdPool.erase(xy_adj_face_id);
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
						Src->FaceIdQueue.push(yz_adj_face_id);
						Src->FaceIdPool.erase(yz_adj_face_id);
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
						Src->FaceIdQueue.push(zx_adj_face_id);
						Src->FaceIdPool.erase(zx_adj_face_id);
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
			else
			{
				CurrentFace.BlackWhite = 1;
			}
		}

		double Step = 0.0;
		if (CurrentFace.IsRead) {
			Src->FaceIdQueue.pop();
			Step = 1.0 / Src->TriangleNum;
		}
		*OutProgressPerRun = Step;
	}
	else {
		if (!Src->FaceIdPool.empty())
		{
			Src->FaceIdQueue.push(*Src->FaceIdPool.begin());
			Src->FaceIdPool.erase(Src->FaceIdPool.begin());
		}
		else {
			*OutProgressPerRun = 1.0;
		}
	}



	if (Src->FaceIdPool.empty() && Src->FaceIdQueue.empty())
		return (void*)Src;
	else
		return nullptr;
}


bool AdjacencyProcesser::GetReady3()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || TriangleBytesData == nullptr || IsWorking())
	{
		ErrorString += "GetReady2 has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (int i = 0; i < TriangleContextList.size(); i++) {
		WRITE_MESSAGE_DIGIT("Mesh: ", i);
		WRITE_MESSAGE_DIGIT("Adj Face Num Before Shrink: ", TriangleContextList[i]->AdjacencyFaceList.size());

	}


	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;
		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}


	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunFunc3, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}


	return true;
}


void* AdjacencyProcesser::RunFunc3(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	AdjFace& CurrentAdjFace = Src->AdjacencyFaceList[Src->CurrentFacePos];

	bool IsAdjWhite1 = true;
	bool IsAdjWhite2 = true;
	bool IsAdjWhite3 = true;
	if (CurrentAdjFace.hasAdjFace[0]) {
		Face& Adj = Src->FaceList[CurrentAdjFace.adjFaceIndex[0]];
		IsAdjWhite1 = (Adj.BlackWhite == -1) ? true : false;
		if (Adj.IsAddToAdjFaceList)
		{
			CurrentAdjFace.hasAdjFace[0] = false;
		}
	}
	if (CurrentAdjFace.hasAdjFace[1]) {
		Face& Adj = Src->FaceList[CurrentAdjFace.adjFaceIndex[1]];
		IsAdjWhite2 = (Adj.BlackWhite == -1) ? true : false;
		if (Adj.IsAddToAdjFaceList)
		{
			CurrentAdjFace.hasAdjFace[1] = false;
		}
	}
	if (CurrentAdjFace.hasAdjFace[2]) {
		Face& Adj = Src->FaceList[CurrentAdjFace.adjFaceIndex[2]];
		IsAdjWhite3 = (Adj.BlackWhite == -1) ? true : false;
		if (Adj.IsAddToAdjFaceList)
		{
			CurrentAdjFace.hasAdjFace[2] = false;
		}
	}
	
	if (IsAdjWhite1 || IsAdjWhite2 || IsAdjWhite3)
	{
		Src->AdjacencyFaceListShrink.push_back(CurrentAdjFace);
		Src->FaceList[CurrentAdjFace.faceIndex].IsAddToAdjFaceList = true;
	}
	else
	{
		Src->FaceList[CurrentAdjFace.faceIndex].BlackWhite = -1;
	}

	Src->CurrentFacePos++;

	double Step = 1.0 / Src->AdjacencyFaceList.size();
	*OutProgressPerRun = Step;

	if (Src->CurrentFacePos >= Src->AdjacencyFaceList.size())
		return (void*)Src;
	else
		return nullptr;
}


bool AdjacencyProcesser::GetReady4()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || IsWorking())
	{
		ErrorString += "GetReady3 has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		AsyncProcesser->AddData((void*)TriangleContextList[i]);
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunFunc4, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}


void* AdjacencyProcesser::RunFunc4(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;

	AdjFace& CurrentAdjFace = Src->AdjacencyFaceListShrink[Src->CurrentFacePos];

	Index x = CurrentAdjFace.x;
	Index y = CurrentAdjFace.y;
	Index z = CurrentAdjFace.z;

	std::unordered_map<uint, std::set<uint>>::iterator it = Src->AdjacencyVertexMap.find(x.value);
	if (it != Src->AdjacencyVertexMap.end())
	{
		std::set<uint>::iterator yit = it->second.find(y.value);
		if (yit == it->second.end()) it->second.insert(y.value);
		std::set<uint>::iterator zit = it->second.find(z.value);
		if (zit == it->second.end()) it->second.insert(z.value);
	}
	else
	{
		Src->AdjacencyVertexMap.insert(std::unordered_map<uint, std::set<uint>>::value_type(x.value, std::set<uint>()));
		Src->AdjacencyVertexMap[x.value].insert(y.value);
		Src->AdjacencyVertexMap[x.value].insert(z.value);
	}

	it = Src->AdjacencyVertexMap.find(y.value);
	if (it != Src->AdjacencyVertexMap.end())
	{
		std::set<uint>::iterator xit = it->second.find(x.value);
		if (xit == it->second.end()) it->second.insert(x.value);
		std::set<uint>::iterator zit = it->second.find(z.value);
		if (zit == it->second.end()) it->second.insert(z.value);
	}
	else
	{
		Src->AdjacencyVertexMap.insert(std::unordered_map<uint, std::set<uint>>::value_type(y.value, std::set<uint>()));
		Src->AdjacencyVertexMap[y.value].insert(x.value);
		Src->AdjacencyVertexMap[y.value].insert(z.value);
	}

	it = Src->AdjacencyVertexMap.find(z.value);
	if (it != Src->AdjacencyVertexMap.end())
	{
		std::set<uint>::iterator xit = it->second.find(x.value);
		if (xit == it->second.end()) it->second.insert(x.value);
		std::set<uint>::iterator yit = it->second.find(y.value);
		if (yit == it->second.end()) it->second.insert(y.value);
	}
	else
	{
		Src->AdjacencyVertexMap.insert(std::unordered_map<uint, std::set<uint>>::value_type(z.value, std::set<uint>()));
		Src->AdjacencyVertexMap[z.value].insert(x.value);
		Src->AdjacencyVertexMap[z.value].insert(y.value);
	}

	Src->CurrentFacePos++;

	double Step = 1.0 / Src->AdjacencyFaceListShrink.size();
	*OutProgressPerRun = Step;

	if (Src->CurrentFacePos >= Src->AdjacencyFaceListShrink.size())
		return (void*)Src;
	else
		return nullptr;
}


bool AdjacencyProcesser::GetReady5()
{
	if (AsyncProcesser == nullptr || TriangleContextList.size() == 0 || IsWorking())
	{
		ErrorString += "GetReady4 has not run yet or is still working.\n";
		return false;
	}
	AsyncProcesser->Clear();

	ErrorString = "";
	MessageString = "";

	for (uint i = 0; i < TriangleContextList.size(); i++)
	{
		TriangleContextList[i]->CurrentFacePos = 0;
		TriangleContextList[i]->CurrentIndexPos = 0;
		TriangleContextList[i]->TotalAdjacencyVertexNum = 0;
		AsyncProcesser->AddData((void*)TriangleContextList[i]);

		WRITE_MESSAGE_DIGIT("AdjVertexMap Num: ", TriangleContextList[i]->AdjacencyVertexMap.size());
	}

	std::function<void* (void*, double*)> Runnable = std::bind(&AdjacencyProcesser::RunFunc5, this, std::placeholders::_1, std::placeholders::_2);
	AsyncProcesser->SetRunFunc(Runnable);

	AsyncProcesser->SetIntervalTime(0.0);

	if (!AsyncProcesser->Kick())
	{
		ErrorString += "Start async request failed.\n";
		return false;
	}

	return true;
}


void* AdjacencyProcesser::RunFunc5(void* SourceData, double* OutProgressPerRun)
{
	SourceContext* Src = (SourceContext*)SourceData;
	VertexContext* VertexData = Src->VertexData;

	AdjVertex& CurrentAdjVertex = VertexData->VertexList[Src->CurrentIndexPos];

	std::unordered_map<uint, std::set<uint>>::iterator it = Src->AdjacencyVertexMap.find(Src->CurrentIndexPos);
	if(it != Src->AdjacencyVertexMap.end())
	{
		uint adjId = (uint)(Src->AdjacencyVertexList.size());
		CurrentAdjVertex.adjId = adjId;
		CurrentAdjVertex.adjSerializedId = Src->TotalAdjacencyVertexNum;
		CurrentAdjVertex.adjNum = (uint)(it->second.size());

		std::vector<uint> AdjList;
		for (std::set<uint>::iterator i = it->second.begin(); i != it->second.end(); i++) {
			AdjList.push_back(*i);
			Src->TotalAdjacencyVertexNum++;
		}
		Src->AdjacencyVertexList.push_back(AdjList);
	}
	else
	{
		ErrorString += "Cannot find vertex in AdjacencyVertexMap. Current Vertex Pos: " + std::to_string(Src->CurrentIndexPos) + "\n";
	}


	Src->CurrentIndexPos++;

	double Step = 1.0 / VertexData->VertexList.size();
	*OutProgressPerRun = Step;

	if (Src->CurrentIndexPos >= VertexData->VertexList.size())
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

		// adjacency vertex begin
		TotalBytesLength += UintBytesSize;// total adjacency vertex num
		TotalBytesLength += TriangleContextList[i]->GetAdjacencyVertexListByteSize();

		// adjacency face begin
		TotalBytesLength += UintBytesSize;// face num
		TotalBytesLength += UintBytesSize;// per struct size
		TotalBytesLength += TriangleContextList[i]->GetAdjacencyFaceListByteSize();
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
		ExportAdjacencyVertexList(Buffer, &BytesOffset, TriangleContextList[i]);
		ExportAdjacencyFaceList(Buffer, &BytesOffset, TriangleContextList[i]);
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
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, AdjVertex::ByteSize());

	for (int j = 0; j < VContext->VertexList.size(); j++)
	{
		const AdjVertex& Curr = VContext->VertexList[j];
		WriteFloatToBytesLittleEndian(Buffer, BytesOffset, Curr.x);
		WriteFloatToBytesLittleEndian(Buffer, BytesOffset, Curr.y);
		WriteFloatToBytesLittleEndian(Buffer, BytesOffset, Curr.z);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.adjSerializedId);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.adjNum);
	}
}

void AdjacencyProcesser::ExportAdjacencyVertexList(Byte* Buffer, uint* BytesOffset, const SourceContext* Context)
{
	WRITE_MESSAGE_DIGIT("AdjVertex Num : ", Context->TotalAdjacencyVertexNum);
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Context->TotalAdjacencyVertexNum);

	uint CheckNum = 0;
	for (int j = 0; j < Context->AdjacencyVertexList.size(); j++)
	{
		const std::vector<uint>& Curr = Context->AdjacencyVertexList[j];
		for (int i = 0; i < Curr.size(); i++)
		{
			WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr[i]);
			CheckNum++;
		}
	}

	if(CheckNum != Context->TotalAdjacencyVertexNum)
		WRITE_MESSAGE("Error : TotalAdjacencyVertexNum != CheckNum", "");
}


void AdjacencyProcesser::ExportAdjacencyFaceList(Byte* Buffer, uint* BytesOffset, const SourceContext* Context)
{
	WRITE_MESSAGE_DIGIT("AdjFace Num : ", Context->AdjacencyFaceListShrink.size());
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Context->AdjacencyFaceListShrink.size());
	WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, AdjFace::ByteSize());

	for (int j = 0; j < Context->AdjacencyFaceListShrink.size(); j++)
	{
		const AdjFace& Curr = Context->AdjacencyFaceListShrink[j];
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.x.value);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.y.value);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.z.value);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.hasAdjFace[0] ? (Curr.adjPoint[0].value + 1) : 0);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.hasAdjFace[1] ? (Curr.adjPoint[1].value + 1) : 0);
		WriteUnsignedIntegerToBytesLittleEndian(Buffer, BytesOffset, Curr.hasAdjFace[2] ? (Curr.adjPoint[2].value + 1) : 0);
	}

}


