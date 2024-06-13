//////////////////////////
//Structures
// 
/////////////////////////

cbuffer Constants
{
    uint TotalAdjacencyTrianglesNum;
    uint SilhouetteEnable;
    uint CreaseEnable;
    uint BorderEnable;
    uint HideVisibleEdge;
    uint HideBackFaceEdge;
    uint HideOccludedEdge;
};

struct AdjVertex
{
    float3 v;
    uint adjSerializedId;
    uint adjNum;

    // meshlet
    uint IsInMeshletBorder1;
    uint IsInMeshletBorder2;
};


struct AdjFace
{
    uint x;
    uint y;
    uint z;
    uint xy;
    uint yz;
    uint zx;

    // meshlet
    uint xyLayer1;
    uint xyLayer2;
    uint yzLayer1;
    uint yzLayer2;
    uint zxLayer1;
    uint zxLayer2;
};

struct Slice
{
    uint Id;
    uint BeginSegmentIndex;
    uint BeginPixel;
};

struct LineSegment
{
    // xy : ndc, after perspective divide , zw = ClipPosition.zw
    float4 NDCPosition[2]; 
    
    // direction 0->1
    float2 Direction;

    // if it is backfacing
    uint BackFacing;

    // id for line segment 
    int Id;

    // v0 runtime index, v1 runtime index
    uint RunTimeVertexIndex[2];

    // meshlet index for layer1 and layer2
    uint MeshletIndex[2];
};

struct VisibleLineSegment
{
    int Id;
    uint PixelLength;
    uint PixelLengthShrink;
    uint PixelLengthShrinkTotal;
    uint ClippedPositionOffset;
    float InvPixelLengthBeforeClippedMinusOne;
};


struct PlainLine
{
    float2 NDCPosition[2];
    
    // line segment pixel position of 2 vertex, range in [0, 1]
    float PixelPosition[2];

    // index in plainline buffer
    uint Id;
    // id for line segment. -1 mean it is clipped
    int LineSegmentId;

    // current vertex runtime index
    uint RunTimeVertexIndex[2]; 
    
    // -1: unlinked
    // 0: link to vertex
    // 1~: slice index
    int LinkState[2]; 
    
    // 0: unvisible
    // 1: visible
    uint Visibility;

    // meshlet index for layer1 and layer2
    uint MeshletIndex[2];


};



struct RunTimeVertexState
{
    // num of adjacency lines
    uint AdjNum;

    // -1: no adjacency line 
    // other: adjacency LineSegment/PlainLine(after generate pass) id
    int Left;
    int Right;

};


struct LineMesh
{
    // mesh vertex NDC position, 2 triangle
    float2 Position[4];

    // mesh uv
    float2 UV[4];

    // curvature between adj lines
    float Curvature[2];

    // id of this line
    uint Id;
};