#pragma once

#include "Utils.h"
#include <assert.h>
#include <unordered_map>
#include <unordered_set>
#include <set>







class Cell
{
public:
	Cell() :
		Index(-1), MeshletIndex(-1), IsFree(true),
		Center(0.0f), Normal(0.0f), Area(0.0f),
		BorderFreeNum(-2)
	{}
	Cell(int _Index, Float3& _Center, Float3& _Normal, float _Area) :
		Index(_Index), MeshletIndex(-1), IsFree(true),
		Center(_Center), Normal(_Normal), Area(_Area),
		BorderFreeNum(-2)
	{}

	void AddBorder(int BorderIndex)
	{
		if (BorderIndex != Index) {
			BorderList.insert(BorderIndex);
			BorderFreeNum = BorderList.size();
		}
	}

public:
	int Index; // index for single triangle or meshlet
	int MeshletIndex;
	bool IsFree; // if or not belong to a meshlet

	Float3 Center;
	Float3 Normal;
	float Area;

	int BorderFreeNum;
	std::unordered_set<int> BorderList;
};

class Meshlet
{
public:
	Meshlet() :
		Center(0.0f),
		Normal(0.0f),
		Area(0.0f)
	{
		MeshletLayerId[0] = -1;
		MeshletLayerId[1] = -1;
	}

	void Clear()
	{
		Center = Float3(0.0f);
		Normal = Float3(0.0f);
		Area = 0.0f;
		CellList.clear();
	}

public:
	Float3 Center;
	Float3 Normal;
	float Area;

	std::vector<int> CellList;

	int MeshletLayerId[2];
};

class MeshletContext
{
public:
	MeshletContext() :
		MaxCellNumInMeshlet(0),
		StepPerRun(0.0f),
		ExpectedRadius(0.0f),
		CurrentCenterSum(0.0f),
		CurrentNormalSum(0.0f),
		CurrentAreaSum(0.0f),
		CurrentCellInMeshletPos(0),
		BestScoreIndex(-1),
		BestFreeBorderIndex(-1),
		MinScore(FLT_MAX),
		MinFreeBorder(INT_MAX)
	{}




	void Clear()
	{
		MaxCellNumInMeshlet = 0;
		StepPerRun = 0.0f;
		ExpectedRadius = 0.0f;

		ClearPerMeshletState();

		CellList.clear();
		MeshletList.clear();
		AliveCellList.clear();
	}

	bool CheckFinishCurrentMeshlet(bool* IsMaxMeshletNum, bool ForceAdd = false)
	{
		bool Added = false;
		if (CurrentMeshlet.CellList.size() >= MaxCellNumInMeshlet || ForceAdd == true)
		{
			if (CurrentMeshlet.CellList.size() > MaxCellNumInMeshlet)
				std::cout << "CurrentMeshlet.CellList.size() > MaxCellNumInMeshlet !!" << std::endl;

			CurrentMeshlet.Center = CurrentCenterSum / CurrentMeshlet.CellList.size();
			CurrentMeshlet.Normal = Normalize(CurrentNormalSum);
			CurrentMeshlet.Area = CurrentAreaSum;
			MeshletList.push_back(CurrentMeshlet);

			//Clear current meshlet
			ClearPerMeshletState();

			Added = true;
		}

		if (Added)
		{
			if (AliveCellList.size() > 0)
			{
				std::set<int>::iterator it = AliveCellList.begin();
				float minLen = FLT_MAX;
				int index = *it;
				//for (; it != AliveCellList.end(); it++) {
				//	float len = Length(CellList[*it].Center-CurrentMeshlet.Center);
				//	if (len < minLen)
				//	{
				//		len = minLen;
				//		index = *it;
				//	}
				//	
				//}
				AddToCurrentMeshlet(index);
			}
			else
			{
				*IsMaxMeshletNum = true;
			}
		}


		return Added;
	}

	bool AddToCurrentMeshlet(int SrcCellIndex)
	{
		Cell& ToAdd = CellList[SrcCellIndex];
		if (ToAdd.IsFree == false) return false;

		ToAdd.MeshletIndex = (int)MeshletList.size();
		ToAdd.IsFree = false;
		CurrentMeshlet.CellList.push_back(SrcCellIndex);

		//update center & area
		CurrentCenterSum = CurrentCenterSum + ToAdd.Center;
		CurrentNormalSum = CurrentNormalSum + ToAdd.Normal;
		CurrentAreaSum = CurrentAreaSum + ToAdd.Area;


		//remove from alive cell list
		std::set<int>::iterator it = AliveCellList.find(SrcCellIndex);
		if (it != AliveCellList.end())
			AliveCellList.erase(it);

		//update border list 
		for (std::unordered_set<int>::iterator it = ToAdd.BorderList.begin(); it != ToAdd.BorderList.end(); it++)
		{
			CellList[*it].BorderFreeNum--;
		}

		ClearPerCellState();

		return true;
	}


	void ClearPerCellState()
	{
		MinScore = FLT_MAX;
		MinFreeBorder = INT_MAX;
		BestScoreIndex = -1;
		BestFreeBorderIndex = -1;
		CurrentCellInMeshletPos = 0;
	}

	void ClearPerMeshletState()
	{
		CurrentCenterSum = Float3(0.0f);
		CurrentNormalSum = Float3(0.0f);
		CurrentAreaSum = 0.0f;
		CurrentMeshlet.Clear();

		ClearPerCellState();
	}


	bool RunStep(double* OutStepPerRun);

public:
	int MaxCellNumInMeshlet;
	float StepPerRun;
	float ExpectedRadius;

	Float3 CurrentCenterSum;
	Float3 CurrentNormalSum;
	float CurrentAreaSum;
	Meshlet CurrentMeshlet;

	int CurrentCellInMeshletPos;
	int BestScoreIndex;
	int BestFreeBorderIndex;
	float MinScore;
	int MinFreeBorder;


public:
	std::vector<Cell> CellList;
	std::vector<Meshlet> MeshletList;

	std::set<int> AliveCellList;

};




struct KDNode
{
	union
	{
		float split;
		unsigned int index;
	};

	// leaves: axis = 3, children = number of extra points after this one (0 if 'index' is the only point)
	// branches: axis != 3, left subtree = skip 1, right subtree = skip 1+children
	unsigned int axis : 2;
	unsigned int children : 30;
};

struct MeshOptTriangle
{
public:
	MeshOptTriangle() :
		Center(0.0f), Normal(0.0f), Area(0.0f)
	{
		index[0] = 0;
		index[1] = 0;
		index[2] = 0;
	}
	MeshOptTriangle(Float3& _Center, Float3& _Normal, float _Area, unsigned int index1, unsigned int index2, unsigned int index3) :
		Center(_Center), Normal(_Normal), Area(_Area)
	{
		index[0] = index1;
		index[1] = index2;
		index[2] = index3;
	}

	Float3 Center;
	Float3 Normal;
	float Area;

	unsigned int index[3];
};
struct MeshOptVertex
{
	MeshOptVertex()
	{
		neighborTriangles.clear();
	}

	std::set<unsigned int> neighborTriangles;
};

struct Cone
{
	float px, py, pz;
	float nx, ny, nz;

	void Clear()
	{
		px = 0;
		py = 0;
		pz = 0;
		nx = 0;
		ny = 0;
		nz = 0;
	}
};

class meshopt_Meshlet
{
public:
	meshopt_Meshlet()
	{
		Clear();
	}

	void Clear()
	{
		vertex_offset = 0;
		triangle_offset = 0;
		vertex_count = 0;
		triangle_count = 0;
	}

public:
	/* offsets within meshlet_vertices and meshlet_triangles arrays with meshlet data */
	unsigned int vertex_offset;
	unsigned int triangle_offset;

	/* number of vertices and triangles used in the meshlet; data is stored in consecutive range defined by offset and count */
	unsigned int vertex_count;
	unsigned int triangle_count;
};

class MeshOpt
{

public:
	MeshOpt():
		mesh_area(0.0f),
		meshlet_expected_radius(0.0),
		cone_weight(0.0),
		max_triangles(0.0),
		max_meshlets(0.0),
		step_per_run(0.0),
		meshlet_offset(0.0),
		emitted_flags(nullptr),
		used(nullptr),
		live_triangles(nullptr),
		kdindices(nullptr),
		nodes(nullptr),
		meshlets_list(nullptr),
		meshlet_vertices(nullptr),
		meshlet_triangles(nullptr)
	{
		meshlet_cone_acc = {};
		meshlet = {};
	}
	~MeshOpt(){
		Clear();
	}

	void Clear()
	{
		if (emitted_flags != nullptr)
			delete[] emitted_flags;
		emitted_flags = nullptr;

		if (used != nullptr)
			delete[] used;
		used = nullptr;

		if (live_triangles != nullptr)
			delete[] live_triangles;
		live_triangles = nullptr;

		if (kdindices != nullptr)
			delete[] kdindices;
		kdindices = nullptr;

		if (nodes != nullptr)
			delete[] nodes;
		nodes = nullptr;

		if (meshlets_list != nullptr)
			delete[] meshlets_list;
		meshlets_list = nullptr;
		meshlet_offset = 0.0;

		if (meshlet_vertices != nullptr)
			delete[] meshlet_vertices;
		meshlet_vertices = nullptr;

		if (meshlet_triangles != nullptr)
			delete[] meshlet_triangles;
		meshlet_triangles = nullptr;

		triangles.clear();
		vertices.clear();

		meshlet_cone_acc.Clear();
		meshlet.Clear();

	}

	void Init(unsigned int vertex_count, unsigned int face_count);
	void GetReady(size_t max_triangles_per_meshlet, unsigned int face_count, unsigned int vertex_count, float normal_weight);
	bool RunStep(double* OutStep);

	size_t GetMeshletSize() const { return meshlet_offset; }

public:
	float mesh_area;
	float meshlet_expected_radius;
	float cone_weight;
	size_t max_triangles;
	size_t max_meshlets;

	std::vector<MeshOptTriangle> triangles;
	std::vector<MeshOptVertex> vertices;

public:
	double step_per_run;
	meshopt_Meshlet* meshlets_list;
	unsigned int* meshlet_vertices;
	unsigned int* meshlet_triangles;
	size_t meshlet_offset;

public:
	Cone meshlet_cone_acc;
	meshopt_Meshlet meshlet;

	unsigned char* emitted_flags;
	unsigned char* used;
	unsigned int* live_triangles;
	unsigned int* kdindices;
	KDNode* nodes;


};