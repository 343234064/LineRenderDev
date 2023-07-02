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
    uint HideBackFaceEdge;
    uint HideOccludedEdge;
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
    uint BeginSegmentIndex;
    uint BeginPixel;

    //temp
    float debug;
};

struct LineSegment
{
    float4 ClipPosition[2]; // after MVP
    float4 NDCPosition[2]; // after perspective divide

    uint BackFacing;
    uint PixelLength;
    uint PixelLengthShrink;
    uint PixelLengthShrinkTotal;
};

struct PlainLine
{
    float2 NDCPosition[2];
    uint BackFacing;
    
    //temp
    uint SliceIndex;
    float debug;
    float debug2;
};