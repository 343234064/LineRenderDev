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
};


struct AdjFace
{
    uint x;
    uint y;
    uint z;
    uint xy;
    uint yz;
    uint zx;
};

struct Slice
{
    uint Id;
    uint BeginSegmentIndex;
    uint BeginPixel;
};

struct LineSegment
{
    float4 ClipPosition[2]; // after MVP
    float4 NDCPosition[2]; // after perspective divide

    // -1: vertices which are added by clipping test
    // other: current vertex index
    int VertexIndex[2];

    uint BackFacing;
    uint PixelLength;
    uint PixelLengthShrink;
    uint PixelLengthShrinkTotal;
};

struct PlainLine
{
    float2 NDCPosition[2];

    // Index in plainline buffer  
    uint Id;

    // -1: vertices which are added by visible test
    // other: current vertex index OR slice index if link to slice
    int VertexIndex[2]; 
    
    // -1: unlinked
    // 0: link to other vertex
    // 1: link to other slice
    int LinkState[2]; 
    
    // 0: unvisible
    // 1: visible
    uint Visibility;
};