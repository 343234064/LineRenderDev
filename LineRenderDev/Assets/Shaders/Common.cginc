//////////////////////////
//Common
// 
/////////////////////////

#if defined(SHADER_API_D3D11) || defined(SHADER_API_PSSL) || defined(SHADER_API_METAL) || defined(SHADER_API_VULKAN) || defined(SHADER_API_SWITCH)
#define FLIPPED_PROJECTION 1
#endif
#if defined(SHADER_API_D3D11) || defined(SHADER_API_PSSL) || defined(SHADER_API_METAL) || defined(SHADER_API_VULKAN) || defined(SHADER_API_SWITCH)
#define REVERSED_Z 1
#endif
#if defined(SHADER_API_D3D11) || defined(SHADER_API_PSSL) || defined(SHADER_API_METAL) || defined(SHADER_API_VULKAN) || defined(SHADER_API_SWITCH)
#define NEGATIVE_CLIP_Z_VALUE 0
#elif defined(SHADER_API_D3D9)  || defined(SHADER_API_WIIU) || defined(SHADER_API_D3D11_9X)
#define NEGATIVE_CLIP_Z_VALUE 0
#else
#define NEGATIVE_CLIP_Z_VALUE 1
#endif


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

struct LineSegment
{
    float3 LocalPosition[2];
    float4 NDCPosition[2];

    int Extracted;
    int Visible;
};

struct Slice
{
    int SegmentIndex;
    int SliceIndex;

    float2 PixelPostion1;
    float2 PixelPostion2;
};



/*
* In NDC space, XY range in [-1.0, 1.0]. (If is opengl, Y should be filpped upside-down)
* In Direct3D, Z range in [0.0, 1.0]. (If is reversed-z, it will be [1.0, 0.0] in unity)
* In OpenGL, Z range in [-1.0, 1.0].
* w might be 0(degenerate case)
*/
inline float3 ComputeScreenPosition(float4 NDCPosition)
{
    float3 ScreenPosition = float3(NDCPosition.x * 0.5f + 0.5f, NDCPosition.y * 0.5f + 0.5f, NDCPosition.z);
#if FLIPPED_PROJECTION
    ScreenPosition.y = 1.0f - ScreenPosition.y;
#endif
#if NEGATIVE_CLIP_Z_VALUE
    ScreenPosition.z = ScreenPosition.z * 0.5f + 0.5f;
#endif
    return ScreenPosition;
}