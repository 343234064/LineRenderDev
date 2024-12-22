//////////////////////////
//Structures
// 
/////////////////////////

cbuffer Constants
{
    uint TotalVerticesNum;
    uint TotalRepeatedVerticesNum;
    uint TotalFacesNum;
    uint TotalMeshletsNum;

    uint ContourEnable;
    uint CreaseEnable;
    uint BorderEnable;
    uint HideVisibleEdge;
    uint HideBackFaceEdge;
    uint HideOccludedEdge;
};




#ifndef ENABLE_DEBUG_VIEW
#define ENABLE_DEBUG_VIEW 0
#endif


//Mesh data////////////////////////////////////////////
struct EXPORTVertex
{
    float3 Position;
    uint UniqueIndex;
};

struct EXPORTFace
{
    uint v012;

    uint edge01;
    uint edge12;
    uint edge20;

    uint UniqueIndex01;
    uint UniqueIndex12;
    uint UniqueIndex20;

    uint3 PackNormalData0;
    uint3 PackNormalData1;

    uint2 MeshletData;
};

struct EXPORTMeshlet
{
    uint FaceOffset;
    uint FaceNum;
    uint VertexOffset;
    uint VertexNum;
};

struct Face
{
    uint v0Offset;
    uint v1Offset;
    uint v2Offset;

    int adjFace01;
    int adjFace12;
    int adjFace20;

    bool e01h;
    bool e01u;

    bool e12h;
    bool e12u;

    bool e20h;
    bool e20u;

    uint edge01;
    uint edge12;
    uint edge20;

    float3 Normal;
    float3 V0Normal;
    float3 V1Normal;
    float3 V2Normal;

    uint2 MeshletData;
};




//Runtime data////////////////////////////////////////////
struct Contour
{
    // 0: Coutour Face 1: Coutour Edge 2: Crease Edge 3: Border Edge 4 : Custom Edge
    // the 4th bit : BackFacing
    uint Type;

    // Type: Face
    // the 32th bit sign for view test
    uint v0;
    uint v1;
    uint v2;

    // Type: Face
    // normals of 3 vertex
    // Type: Edge
    // position of v0, v1
    float3 V0Data;
    float3 V1Data;
    float3 V2Data;

    // 0 : Head 1 : Tail
    // 1~ 32 bit : Anchor Index 
    uint Anchor[2];

    // 0 : meshlet layer1 index
    // 1 : meshlet layer2 index
    uint MeshletData[2];
};



struct SegmentMetaData
{
    // xy : Clipped NDC.xy  zw : ClipSpace.zw 
    float4 NDCPosition[2];
#if ENABLE_DEBUG_VIEW
    float4 NDCPositionForDebug[2];
#endif


    // Visible pixel length with clipping 
    uint PixelLength;

    // Visible (pixel length / 2) with clipping 
    uint PixelLengthShrink;
    uint PixelLengthShrinkTotal;

    // [1st ~ 3rd bit] :  0: Coutour Face 1: Coutour Edge 2: Crease 3: Border 4 : Custom Edge
    // the 4th bit : BackFacing
    uint Type;

    // use for calculate pixel position
    uint ClippedPositionOffset;
    float InvPixelLengthBeforeClippedMinusOne;

    // 0 : Head 1 : Tail
    // 1~ 32 bit : Anchor Index 
    uint Anchor[2];

    // 0 : meshlet layer1 index
    // 1 : meshlet layer2 index
    uint MeshletData[2];
};


struct Segment
{
    // xy : NDC.xy
    float2 NDCPosition[2];
#if ENABLE_DEBUG_VIEW
    float4 NDCPositionForDebug[2];
#endif

    // pixel position for 2 vertex in total segment length
    // range 0-1
    float PixelPosition[2];

    // [1st ~ 3rd bit] :  0: Coutour Face 1: Coutour Edge 2: Crease 3: Border
    // the 4th bit : BackFacing
    // the 5th bit : Visibility
    uint Type;


    // Slice Id
    uint SliceId;
    
    // SegmentMetaData Id
    // 0xffffffff : unexist
    uint SegmentMetaDataId;

    // 0 : Head 1 : Tail
    // 1~ 30 bit : Anchor Index 
    // 31 ~ 32bit : 0 -> cut by visibility test(ztest)
    //                     1 -> vertex
    //                     2 -> slice
    uint Anchor[2];

    // 0 : meshlet layer1 index
    // 1 : meshlet layer2 index
    uint MeshletData[2];
};



/*
struct SliceMetaData
{
    uint Type;
    uint SegmentMetaDataId;
    uint Anchor;
    uint SegmentId;
    uint SliceId;
    float2 NDCPosition;
    float PixelPosition;
#if ENABLE_DEBUG_VIEW
    float4 DebugPosition;
#endif
};*/

struct Slice
{
    uint Id;
    uint BeginSegmentIndex;
    uint BeginPixel;

    //SliceMetaData HeadTailSegment[2];
};






struct LineHead
{
    // 1~31bit : LineMeshId
    // the 32 bit: start point  
    // 0->v0 1->v1
    uint Data;
};


//
// x : adj edge num
// y : 1~31bit : adj edge index
// z : 1~31bit : adj edge index
// 32bit : 
// 0->connect to adjacent contour's v0
// 1->connect to adjacent contour's v1
//0xffffffff : unexist
struct AnchorEdge
{
    uint3 ContourType;
};
struct AnchorVertex
{
    uint3 ContourType;
    uint3 CreaseType;
};
// without adj edge num
struct AnchorSlice
{
    uint2 AllType;
};



struct LineMesh
{
    // mesh vertex NDC position
    float2 NDCPosition[2];
#if ENABLE_DEBUG_VIEW
    float4 NDCPositionForDebug[2];

    // Slice Id
    // the 32th bit sign for IsSliceTail
    // the 31th bit sign for IsSliceHead
    uint SliceId;
#endif

    // pixel position for 2 vertex in total segment length
    // range 0-1
    float PixelPosition[2];

    // [1st ~ 3rd bit] :  0: Coutour Face 1: Coutour Edge 2: Crease 3: Border
    // the 4th bit : BackFacing
    // the 5th bit : Visibility
    uint Type;

    // 1~31bit:
    //  0->v0 adjacent contour index 
    //  1->v1 adjacent contour index
    // 32bit : 
    // 0->connect to adjacent contour's v0
    // 1->connect to adjacent contour's v1
    // 0xffffffff: unexist
    uint LinkId[2];

    // 0 : Head 1 : Tail
    // 1~ 30 bit : Anchor Index 
    // 31 ~ 32bit : 0 -> cut by visibility test(ztest)
    //                     1 -> vertex
    //                     2 -> slice
    uint Anchor[2];

    // 0 : meshlet layer1 index
    // 1 : meshlet layer2 index
    uint MeshletData[2];

    // id of the whole line which this LineMesh belong to
    // xy : two direction, forward or backward
    float GroupId[2];


};


#define CONTOUR_FACE_TYPE 0
#define CONTOUR_EDGE_TYPE 1
#define CREASE_EDGE_TYPE 2
#define BORDER_EDGE_TYPE 3
#define CUSTOM_EDGE_TYPE 4
#define DEBUG_TYPE 7

#define GET_LINE_TYPE(Dest) (Dest & 0x7)
#define GET_LINE_IS_BACKFACING(Dest) ((Dest & 0x8) > 0)
#define GET_LINE_IS_VISIBLE(Dest) ((Dest & 0x10) > 0)
#define SET_LINE_TYPE(Dest, Value) ((Dest & ~0x7) | ((Value) & 0x7))
#define SET_LINE_IS_BACKFACING(Dest, Value)  ((Value) ? (Dest | 0x8) : (Dest & ~0x8))
#define SET_LINE_IS_VISIBLE(Dest, Value) ((Value) ? (Dest | 0x10) : (Dest & ~0x10))

#define SET_LINE_DEBUG_TYPE 0xffffffff

/*
#define GET_SLICE_ID(Dest) (Dest & 0x3fffffff)
#define GET_IS_SLICE_HEAD(Dest) ((Dest & 0x40000000) > 0)
#define GET_IS_SLICE_TAIL(Dest) ((Dest & 0x80000000) > 0)
#define SET_SLICE_ID(Dest, Value) ((Dest & ~0x3fffffff) | ((Value) & 0x3fffffff))
#define SET_IS_SLICE_HEAD(Dest, Value) ((Value) ? (Dest | 0x40000000) : (Dest & ~0x40000000))
#define SET_IS_SLICE_TAIL(Dest, Value) ((Value) ? (Dest | 0x80000000) : (Dest & ~0x80000000))
*/

#define GET_ANCHOR_INDEX(Dest) (Dest & 0x3fffffff)
#define GET_ANCHOR_TYPE(Dest) (Dest >> 30)
#define SET_ANCHOR_INDEX(Dest, Value) ((Dest & ~0x3fffffff) | ((Value) & 0x3fffffff))
#define SET_ANCHOR_TYPE(Dest, Value) ((Dest & 0x3fffffff) | ((Value) << 30))

#define GET_ADJACENT_INDEX(Dest) (Dest & 0x7fffffff)
#define GET_ADJACENT_DIRECTION(Dest) ((Dest & 0x80000000) > 0 ? 1 : 0)
#define SET_ADJACENT_INDEX(Dest, Value)  ((Dest & ~0x7fffffff) | ((Value) & 0x7fffffff))
#define SET_ADJACENT_DIRECTION(Dest, Value) ((Value) ? (Dest | 0x80000000) : (Dest & ~0x80000000))

#define GET_LINEHEAD_INDEX(Dest) (Dest & 0x7fffffff)
#define GET_LINEHEAD_DIRECTION(Dest) ((Dest & 0x80000000) > 0 ? 1 : 0)
#define SET_LINEHEAD_INDEX(Dest, Value)  ((Dest & ~0x7fffffff) | ((Value) & 0x7fffffff))
#define SET_LINEHEAD_DIRECTION(Dest, Value) ((Value) ? (Dest | 0x80000000) : (Dest & ~0x80000000))