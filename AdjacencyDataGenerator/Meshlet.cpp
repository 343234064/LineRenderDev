
#include "Meshlet.h"

bool MeshletContext::RunStep(double* OutStepPerRun)
{

	if (CurrentMeshlet.CellList.size() == 0) return  true;

	bool IsMaxMeshletNum = false;
	bool AddedMeshlet = false;
	bool AddedCell = false;
	if (CurrentCellInMeshletPos >= CurrentMeshlet.CellList.size())
	{
		bool found = false;
		if (BestScoreIndex != -1 || BestFreeBorderIndex != -1)
		{
			int BestIndex = BestFreeBorderIndex != -1 ? BestFreeBorderIndex : BestScoreIndex;

			AddedCell = AddToCurrentMeshlet(BestIndex);
			found = true;
		}
		AddedMeshlet = CheckFinishCurrentMeshlet(&IsMaxMeshletNum, !found);
		CurrentCellInMeshletPos = 0;
	}
	else
	{
		int CellInMeshletIndex = CurrentMeshlet.CellList[CurrentCellInMeshletPos];
		Cell& CellInMeshlet = CellList[CellInMeshletIndex];

		if (CellInMeshlet.BorderList.size() > 0) {
			Float3 MeshletCenter = CurrentCenterSum / CurrentMeshlet.CellList.size();
			Float3 MeshletNormal = Normalize(CurrentNormalSum);

			for (std::unordered_set<int>::iterator it = CellInMeshlet.BorderList.begin(); it != CellInMeshlet.BorderList.end(); it++)
			{
				Cell& AdjCell = CellList[*it];
				if (AdjCell.IsFree)
				{
					float Distance = Length(AdjCell.Center - MeshletCenter);
					float Angle = Dot(AdjCell.Normal, MeshletNormal);

					float AngleWeight = 0.0f;
					float AngleScore = 1.0f - Angle * AngleWeight;
					AngleScore = AngleScore < 1e-3f ? 1e-3f : AngleScore;

					float Score = (1.0f + Distance / ExpectedRadius * (1.0f - AngleWeight)) * AngleScore;

					if (Score < MinScore)
					{
						MinScore = Score;
						BestScoreIndex = *it;
					}

					if (AdjCell.BorderFreeNum >= 0 && AdjCell.BorderFreeNum < 5 && AdjCell.BorderFreeNum < MinFreeBorder)
					{
						BestFreeBorderIndex = *it;
						MinFreeBorder = AdjCell.BorderFreeNum;
					}

				}

			}

		}

		CurrentCellInMeshletPos++;
	}


	if (AddedCell)
	{
		*OutStepPerRun = StepPerRun;
	}
	else
	{
		*OutStepPerRun = 0.0f;
	}

	if (IsMaxMeshletNum) {
		if (AliveCellList.size() != 0)
			std::cout << "AliveCellList.size() != 0 !!" << std::endl;
		return true;
	}
	else
		return false;
}













size_t kdtreePartition(unsigned int* indices, size_t count, const std::vector<MeshOptTriangle>* triangles, unsigned int axis, float pivot)
{
	size_t m = 0;

	// invariant: elements in range [0, m) are < pivot, elements in range [m, i) are >= pivot
	for (size_t i = 0; i < count; ++i)
	{

		float v = (*triangles)[indices[i]].Center[axis];

		// swap(m, i) unconditionally
		unsigned int t = indices[m];
		indices[m] = indices[i];
		indices[i] = t;

		// when v >= pivot, we swap i with m without advancing it, preserving invariants
		m += v < pivot;
	}

	return m;
}


size_t kdtreeBuildLeaf(size_t offset, KDNode* nodes, size_t node_count, unsigned int* indices, size_t count)
{
	assert(offset + count <= node_count);
	(void)node_count;

	KDNode& result = nodes[offset];

	result.index = indices[0];
	result.axis = 3;
	result.children = unsigned(count - 1);

	// all remaining points are stored in nodes immediately following the leaf
	for (size_t i = 1; i < count; ++i)
	{
		KDNode& tail = nodes[offset + i];

		tail.index = indices[i];
		tail.axis = 3;
		tail.children = ~0u >> 2; // bogus value to prevent misuse
	}

	return offset + count;
}

size_t kdtreeBuild(size_t offset, KDNode* nodes, size_t node_count, const std::vector<MeshOptTriangle>* triangles, unsigned int* indices, size_t count, size_t leaf_size)
{
	assert(count > 0);
	assert(offset < node_count);

	if (count <= leaf_size)
		return kdtreeBuildLeaf(offset, nodes, node_count, indices, count);

	float mean[3] = {};
	float vars[3] = {};
	float runc = 1, runs = 1;

	// gather statistics on the points in the subtree using Welford's algorithm
	for (size_t i = 0; i < count; ++i, runc += 1.f, runs = 1.f / runc)
	{
		const Float3 point = (*triangles)[indices[i]].Center;

		for (int k = 0; k < 3; ++k)
		{
			float delta = point[k] - mean[k];
			mean[k] += delta * runs;
			vars[k] += delta * (point[k] - mean[k]);
		}
	}

	// split axis is one where the variance is largest
	unsigned int axis = vars[0] >= vars[1] && vars[0] >= vars[2] ? 0 : vars[1] >= vars[2] ? 1 : 2;

	float split = mean[axis];
	size_t middle = kdtreePartition(indices, count, triangles, axis, split);

	// when the partition is degenerate simply consolidate the points into a single node
	if (middle <= leaf_size / 2 || middle >= count - leaf_size / 2)
		return kdtreeBuildLeaf(offset, nodes, node_count, indices, count);

	KDNode& result = nodes[offset];

	result.split = split;
	result.axis = axis;

	// left subtree is right after our node
	size_t next_offset = kdtreeBuild(offset + 1, nodes, node_count, triangles, indices, middle, leaf_size);

	// distance to the right subtree is represented explicitly
	result.children = unsigned(next_offset - offset - 1);

	return kdtreeBuild(next_offset, nodes, node_count, triangles, indices + middle, count - middle, leaf_size);
}

void kdtreeNearest(KDNode* nodes, unsigned int root, const std::vector<MeshOptTriangle>* triangles, const unsigned char* emitted_flags, const float* position, unsigned int& result, float& limit)
{
	const KDNode& node = nodes[root];

	if (node.axis == 3)
	{
		// leaf
		for (unsigned int i = 0; i <= node.children; ++i)
		{
			unsigned int index = nodes[root + i].index;

			if (emitted_flags[index])
				continue;

			const Float3 point = (*triangles)[index].Center;

			float distance2 =
				(point[0] - position[0]) * (point[0] - position[0]) +
				(point[1] - position[1]) * (point[1] - position[1]) +
				(point[2] - position[2]) * (point[2] - position[2]);
			float distance = sqrtf(distance2);

			if (distance < limit)
			{
				result = index;
				limit = distance;
			}
		}
	}
	else
	{
		// branch; we order recursion to process the node that search position is in first
		float delta = position[node.axis] - node.split;
		unsigned int first = (delta <= 0) ? 0 : node.children;
		unsigned int second = first ^ node.children;

		kdtreeNearest(nodes, root + 1 + first, triangles, emitted_flags, position, result, limit);

		// only process the other node if it can have a match based on closest distance so far
		if (fabsf(delta) <= limit)
			kdtreeNearest(nodes, root + 1 + second, triangles, emitted_flags, position, result, limit);
	}
}


inline bool CheckCondition(meshopt_Meshlet& meshlet, size_t max_triangles)
{
	return meshlet.triangle_count >= max_triangles;
}


void finishMeshlet(size_t t, meshopt_Meshlet& meshlet, std::vector<unsigned int>* meshlet_vertices, Cone* meshlet_cone, unsigned char* used, unsigned int* vertex_meshlet_occupy_flag)
{
	//size_t offset = meshlet.triangle_offset + meshlet.triangle_count * 3;

	//// fill 4b padding with 0
	//while (offset & 3)
	//	meshlet_triangles[offset++] = 0;

	for (size_t j = 0; j < meshlet.vertex_count; ++j) {
		unsigned int Index = (*meshlet_vertices)[meshlet.vertex_offset + j];
		used[Index] = 0xff;
		vertex_meshlet_occupy_flag[Index] += 1;
	}

	meshlet.Center = Float3(meshlet_cone->px, meshlet_cone->py, meshlet_cone->pz);
	meshlet.Normal = Float3(meshlet_cone->nx, meshlet_cone->ny, meshlet_cone->nz);
	meshlet.Area = meshlet_cone->area;

}

bool appendMeshlet(size_t t, meshopt_Meshlet& meshlet, std::set<unsigned int>* indexes, unsigned int best_triangle,
	unsigned char* used, unsigned int* vertex_meshlet_occupy_flag,
	std::vector<meshopt_Meshlet>* meshlets, std::vector<unsigned int>* meshlet_vertices, std::vector<unsigned int>* meshlet_triangles,
	size_t meshlet_offset, size_t max_triangles, Cone* meshlet_cone, bool force_finish)
{

	bool result = false;

	if (CheckCondition(meshlet, max_triangles) || (force_finish && meshlet.triangle_count > 0))
	{
		finishMeshlet(t, meshlet, meshlet_vertices, meshlet_cone, used, vertex_meshlet_occupy_flag);
		//meshlets[meshlet_offset] = meshlet;
		meshlets->push_back(meshlet);

		meshlet.vertex_offset += meshlet.vertex_count;
		meshlet.triangle_offset += meshlet.triangle_count; 
		meshlet.vertex_count = 0;
		meshlet.triangle_count = 0;
		meshlet.ClearLayerData();

		result = true;
	}


	for (std::set<unsigned int>::iterator it = indexes->begin(); it != indexes->end(); it++) {
		unsigned char& v = used[*it];
		if (v == 0xff)
		{
			//meshlet_vertices[meshlet.vertex_offset + meshlet.vertex_count++] = *it;
			meshlet_vertices->push_back(*it);
			meshlet.vertex_count++;
		}
	}


	//meshlet_triangles[meshlet.triangle_offset + meshlet.triangle_count] = best_triangle;
	meshlet_triangles->push_back(best_triangle);
	meshlet.triangle_count++;

	return result;
}

Cone getMeshletCone(const Cone& acc, unsigned int triangle_count)
{
	Cone result = acc;

	float center_scale = triangle_count == 0 ? 0.f : 1.f / float(triangle_count);

	result.px *= center_scale;
	result.py *= center_scale;
	result.pz *= center_scale;

	float axis_length = result.nx * result.nx + result.ny * result.ny + result.nz * result.nz;
	float axis_scale = axis_length == 0.f ? 0.f : 1.f / sqrtf(axis_length);

	result.nx *= axis_scale;
	result.ny *= axis_scale;
	result.nz *= axis_scale;

	return result;
}

float getMeshletScore(float distance2, float spread, float cone_weight, float expected_radius)
{
	float cone = 1.f - spread * cone_weight;
	float cone_clamped = cone < 1e-3f ? 1e-3f : cone;

	return (1 + sqrtf(distance2) / expected_radius * (1 - cone_weight)) * cone_clamped;
}

unsigned int getNeighborTriangle(const meshopt_Meshlet& meshlet, const Cone* meshlet_cone, std::vector<unsigned int>* meshlet_vertices, 
	const std::vector<MeshOptVertex>* vertices, const std::vector<MeshOptTriangle>* triangles, 
	const unsigned int* live_triangles, const unsigned char* used, float meshlet_expected_radius, float cone_weight)
{
	unsigned int best_triangle = ~0u;
	unsigned int best_extra = INT_MAX; //5
	float best_score = FLT_MAX;

	for (size_t i = 0; i < meshlet.vertex_count; ++i)
	{
		unsigned int index = (*meshlet_vertices)[meshlet.vertex_offset + i];

		const std::set<unsigned int>& neighborTriangles = (*vertices)[index].neighborTriangles;

		for (std::set<unsigned int>::iterator j = neighborTriangles.begin(); j != neighborTriangles.end(); ++j)
		{
			unsigned int triangleIndex = *j;
			const MeshOptTriangle& tri = (*triangles)[*j];

			unsigned int extra = 0;
			bool has_one_live_triangle = false;
			float live_triangle_score = 0;
			for (std::set<unsigned int>::iterator it = tri.indexes.begin(); it != tri.indexes.end(); it++) {
				extra += (used[*it] == 0xff);
				if (live_triangles[*it] == 1)
					has_one_live_triangle = true;
				live_triangle_score += float(live_triangles[*it]);
			}

			// triangles that don't add new vertices to meshlets are max. priority
			if (extra != 0)
			{
				// artificially increase the priority of dangling triangles as they're expensive to add to new meshlets
				if (has_one_live_triangle)
					extra = 0;

				extra++;
			}

			// since topology-based priority is always more important than the score, we can skip scoring in some cases
			if (extra > best_extra)
				continue;

			float score = 0;

			// caller selects one of two scoring functions: geometrical (based on meshlet cone) or topological (based on remaining triangles)
			if (meshlet_cone)
			{
				float distance2 =
					(tri.Center.x - meshlet_cone->px) * (tri.Center.x - meshlet_cone->px) +
					(tri.Center.y - meshlet_cone->py) * (tri.Center.y - meshlet_cone->py) +
					(tri.Center.z - meshlet_cone->pz) * (tri.Center.z - meshlet_cone->pz);

				float spread = tri.Normal.x * meshlet_cone->nx + tri.Normal.y * meshlet_cone->ny + tri.Normal.z * meshlet_cone->nz;

				score = getMeshletScore(distance2, spread, cone_weight, meshlet_expected_radius);

			}
			else
			{
				// each live_triangles entry is >= 1 since it includes the current triangle we're processing
				score = float(live_triangle_score - tri.indexes.size());
				assert(score >= 0.0f);
			}

			// note that topology-based priority is always more important than the score
			// this helps maintain reasonable effectiveness of meshlet data and reduces scoring cost
			if (extra < best_extra || score < best_score)
			{
				best_triangle = triangleIndex;
				best_extra = extra;
				best_score = score;

			}
		}
	}


	return best_triangle;
}


void MeshOpt::Init(unsigned int vertex_count, unsigned int face_count)
{
	Clear();

	emitted_flags = new unsigned char[face_count];
	memset(emitted_flags, 0, face_count);

	used = new unsigned char[vertex_count];
	memset(used, -1, vertex_count);

	live_triangles = new unsigned int[vertex_count];
	memset(live_triangles, 0, vertex_count * sizeof(unsigned int));

	kdindices = new unsigned int[face_count];
	nodes = new KDNode[size_t(face_count) * 2];
	
	vertex_meshlet_occupy_flag = new unsigned int[vertex_count];
	memset(vertex_meshlet_occupy_flag, 0, vertex_count * sizeof(unsigned int));

	vertices.resize(vertex_count);
}

void MeshOpt::GetReady(size_t max_triangles_per_meshlet, unsigned int face_count, unsigned int vertex_count, float normal_weight)
{
	cone_weight = normal_weight;
	max_triangles = MIN(512, max_triangles_per_meshlet);

	float triangle_area_avg = face_count == 0 ? 0.f : mesh_area / float(face_count) * 0.5f;
	meshlet_expected_radius = sqrtf(triangle_area_avg * max_triangles) * 0.5f;

	//size_t index_count = size_t(face_count) * 3;
	//size_t meshlet_limit_triangles = (index_count / 3 + max_triangles - 1) / max_triangles;
	//max_meshlets = meshlet_limit_triangles;

	meshlets_list.clear();
	//meshlets_list = new meshopt_Meshlet[max_meshlets];
	//memset(meshlets_list, 0, max_meshlets * sizeof(meshopt_Meshlet));

	meshlet_vertices.clear();
	//meshlet_vertices = new unsigned int[vertex_count];
	//memset(meshlet_vertices, 0, vertex_count * sizeof(unsigned int));

	meshlet_triangles.clear();
	//meshlet_triangles = new unsigned int[max_meshlets * max_triangles];
	//memset(meshlet_triangles, 0, max_meshlets * max_triangles * sizeof(unsigned int));


	step_per_run = 1.0 / (face_count + 1.0);//+1 for the last empty run of RunStep()

	//std::cout << "Expected Max Meshlet Num : " << max_meshlets << std::endl;


	kdtreeBuild(0, nodes, size_t(face_count) * 2, &triangles, kdindices, face_count, /* leaf_size= */ 8);

}



bool MeshOpt::RunStep(double* OutStep, bool NeedForceFinish)
{
	Cone meshlet_cone = getMeshletCone(meshlet_cone_acc, meshlet.triangle_count);

	unsigned int best_triangle = getNeighborTriangle(meshlet, &meshlet_cone, &meshlet_vertices, &vertices, &triangles, live_triangles, used, meshlet_expected_radius, cone_weight);

	// if the best triangle doesn't fit into current meshlet, the spatial scoring we've used is not very meaningful, so we re-select using topological scoring
	if (best_triangle != ~0u && (CheckCondition(meshlet, max_triangles)))
	{
		best_triangle = getNeighborTriangle(meshlet, NULL, &meshlet_vertices, &vertices, &triangles, live_triangles, used, meshlet_expected_radius, 0.0f);
	}

	// when we run out of neighboring triangles we need to switch to spatial search; we currently just pick the closest triangle irrespective of connectivity
	bool force_next = false;
	if (best_triangle == ~0u)
	{
		float position[3] = { meshlet_cone.px, meshlet_cone.py, meshlet_cone.pz };
		unsigned int index = ~0u;
		float limit = FLT_MAX;

		kdtreeNearest(nodes, 0, &triangles, emitted_flags, position, index, limit);

		best_triangle = index;
		if (best_triangle != ~0u && NeedForceFinish)
			force_next = true;
	}

	
	if (best_triangle == ~0u) {

		if (meshlet.triangle_count)
		{
			finishMeshlet(vertices.size(), meshlet, &meshlet_vertices, &meshlet_cone, used, vertex_meshlet_occupy_flag);

			//assert(meshlet_offset < max_meshlets);
			meshlets_list.push_back(meshlet);
			meshlet_offset++;
		}
		*OutStep = step_per_run;
		return true;
	}


	// add meshlet to the output; when the current meshlet is full we reset the accumulated bounds
	if (appendMeshlet(vertices.size(), meshlet, &triangles[best_triangle].indexes, best_triangle, used, vertex_meshlet_occupy_flag, &meshlets_list, &meshlet_vertices, &meshlet_triangles, meshlet_offset, max_triangles, &meshlet_cone, force_next))
	{
		meshlet_offset++;
		memset(&meshlet_cone_acc, 0, sizeof(meshlet_cone_acc));
	}


	for (std::set<unsigned int>::iterator it = triangles[best_triangle].indexes.begin(); it != triangles[best_triangle].indexes.end(); it++) {
		live_triangles[*it]--;

		// remove emitted triangle from adjacency data
		// this makes sure that we spend less time traversing these lists on subsequent iterations
		unsigned int index = *it;

		std::set<unsigned int>& neighborTriangles = vertices[index].neighborTriangles;
		for (std::set<unsigned int>::iterator j = neighborTriangles.begin(); j != neighborTriangles.end(); ++j)
		{
			unsigned int tri = *j;

			if (tri == best_triangle)
			{
				neighborTriangles.erase(*j);
				break;
			}
		}
	}


	// update aggregated meshlet cone data for scoring subsequent triangles
	meshlet_cone_acc.px += triangles[best_triangle].Center.x;
	meshlet_cone_acc.py += triangles[best_triangle].Center.y;
	meshlet_cone_acc.pz += triangles[best_triangle].Center.z;
	meshlet_cone_acc.nx += triangles[best_triangle].Normal.x;
	meshlet_cone_acc.ny += triangles[best_triangle].Normal.y;
	meshlet_cone_acc.nz += triangles[best_triangle].Normal.z;
	meshlet_cone_acc.area += triangles[best_triangle].Area;
	emitted_flags[best_triangle] = 1;

	*OutStep = step_per_run;

	return false;
}