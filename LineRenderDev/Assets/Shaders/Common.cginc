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


#define EXTRACT_PASS_PER_THREAD_ELEMENT_NUM 256
// Must be power of 2
// Slice pass max handle num : BUCKET_ELEMENT_NUM * BUCKET_ELEMENT_NUM
#define SLICE_PASS_PER_THREAD_ELEMENT_NUM 1024

#define SLICE_PIXEL_SIZE 64
#define VISIBILITY_PASS_PER_THREAD_ELEMENT_NUM 1


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
    float3 ScreenPosition[2];

    uint BackFacing;
    uint PixelLength;
    uint PixelLengthTotal;
};

struct Slice
{
    uint Visibility1;
    uint Visibility2;

    uint BeginSegmentIndex;
    uint BeginPixel;
};



/*
* In NDC space, XY range in [-1.0, 1.0]. (If is opengl, Y should be filpped upside-down)
* In Direct3D, Z range in [0.0, 1.0]. (If is reversed-z, it will be [1.0, 0.0] in unity)
* In OpenGL, Z range in [-1.0, 1.0].
* w might be 0(degenerate case)
*/
float3 CalculateScreenPosition(float4 NDCPosition)
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



inline bool ZTest(float PositionDepth, float SceneDepth)
{
#if REVERSED_Z
    return (PositionDepth >= SceneDepth) ? true : false;
#else
    return (PositionDepth <= SceneDepth) ? true : false;
#endif
}