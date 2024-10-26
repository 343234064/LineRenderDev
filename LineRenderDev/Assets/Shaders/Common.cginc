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

#define CONTOURIZE_PASS_PER_THREAD_ELEMENT_NUM 256


// Must be power of 2
// Slice pass max handle num : BUCKET_ELEMENT_NUM * BUCKET_ELEMENT_NUM
#define SLICE_PASS_PER_THREAD_ELEMENT_NUM 1024

// 
#define SLICE_PIXEL_SIZE 64
#define VISIBILITY_PASS_PER_THREAD_ELEMENT_NUM 64
#define VISIBILITY_CACHE_NUM 2 // 64/32


#define GENERATE_PASS_PER_THREAD_ELEMENT_NUM 256



/*
* ArgBuffer Offset
* 
*/
#define INDIRECT_DRAW_START 0
#define INDIRECT_DRAW_VERTEX_COUNT INDIRECT_DRAW_START
#define INDIRECT_DRAW_TRIANGLE_COUNT INDIRECT_DRAW_START+1
#define CONTOURIZE_PASS_START 4
#define CONTOURIZE_PASS_DISPATCH_COUNT CONTOURIZE_PASS_START
#define CONTOURIZE_PASS_FACE_COUNT CONTOURIZE_PASS_START+3


#include "Structures.cginc"


/*
* In NDC space, XY range in [-1.0, 1.0]. (If is opengl, Y should be filpped upside-down)
* In Direct3D, Z range in [0.0, 1.0]. (If is reversed-z, it will be [1.0, 0.0] in unity)
* In OpenGL, Z range in [-1.0, 1.0].
* w might be 0(degenerate case)
*/
inline float4 CalculateNDCPosition(float4 ClipPosition)
{
    float4 Projected = float4(ClipPosition.xyz / ClipPosition.w, ClipPosition.w);

    return Projected;
}
inline float2 CalculateNDCPositionXY(float4 ClipPosition)
{
    return float2(ClipPosition.xy / ClipPosition.w);
}

float4 UnifyNDCPosition(float4 NDCPosition)
{
    float4 UnifyNDCPosition = float4(NDCPosition.x * 0.5f + 0.5f, NDCPosition.y * 0.5f + 0.5f, NDCPosition.z, NDCPosition.w);
#if FLIPPED_PROJECTION
    UnifyNDCPosition.y = 1.0f - UnifyNDCPosition.y;
#endif
#if NEGATIVE_CLIP_Z_VALUE
    UnifyNDCPosition.z = UnifyNDCPosition.z * 0.5f + 0.5f;
#endif
    return UnifyNDCPosition;
}
float2 UnifyNDCPositionXY(float2 NDCPosition)
{
    float2 UnifyNDCPosition = float2(NDCPosition.x * 0.5f + 0.5f, NDCPosition.y * 0.5f + 0.5f);
#if FLIPPED_PROJECTION
    UnifyNDCPosition.y = 1.0f - UnifyNDCPosition.y;
#endif

    return UnifyNDCPosition;
}



float4 ReverseUnifyNDCPosition(float4 UnifyNDCPosition)
{
    float4 NDCPosition = UnifyNDCPosition;

#if FLIPPED_PROJECTION
    NDCPosition.y = 1.0f - UnifyNDCPosition.y;
#endif
#if NEGATIVE_CLIP_Z_VALUE
    NDCPosition.z = (NDCPosition.z - 0.5f) * 2.0f;
#endif

    NDCPosition = float4((NDCPosition.x - 0.5f) * 2.0f, (NDCPosition.y - 0.5f) * 2.0f, NDCPosition.z, NDCPosition.w);

    return NDCPosition;
}

inline float GetUnifyNDCZ(float NDCZ)
{
    float Z = NDCZ;
#if NEGATIVE_CLIP_Z_VALUE
    Z = NDCZ * 0.5f + 0.5f;
#endif
    return Z;
}

inline bool ZTest(float PositionDepth, float SceneDepth)
{
#if REVERSED_Z
    return (PositionDepth > SceneDepth) ? true : false;
#else
    return (PositionDepth < SceneDepth) ? true : false;
#endif
}

inline float Interpolate(float V0Attribute, float V1Attribute, float TFromV0)
{
    float T = saturate(TFromV0);
    return V0Attribute * (1.0f - T) + V1Attribute * T;
}

inline float2 Interpolate(float2 V0Attribute, float2 V1Attribute, float TFromV0)
{
    float T = saturate(TFromV0);
    return V0Attribute * (1.0f - T) + V1Attribute * T;
}


float3 Hash31(float p)
{
    float3 p3 = frac(float3(p, p, p) * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.xxy + p3.yzz) * p3.zyx);
}




void UnpackUintToFloats(uint v, out float a, out float b)
{
    const float Scaler = 1.0f / 65535.0f;
    uint uintInput = v;
    a = (uintInput >> 16) * Scaler;
    b = (uintInput & 0xFFFF) * Scaler;
}

void UnpackUint3ToNormals(uint3 V, out float3 N1, out float3 N2)
{
    float3 R1 = 0.0f;
    float3 R2 = 0.0f;
    UnpackUintToFloats(V.x, R1.x, R1.y);
    UnpackUintToFloats(V.y, R1.z, R2.x);
    UnpackUintToFloats(V.z, R2.y, R2.z);

    N1 = (R1 * 2.0f - 1.0f);
    N2 = (R2 * 2.0f - 1.0f);

}

float3 GetFaceNormal(EXPORTFace InData)
{
    float3 R1 = 0.0f;
    float R2 = 0.0f;
    UnpackUintToFloats(InData.PackNormalData0.x, R1.x, R1.y);
    UnpackUintToFloats(InData.PackNormalData0.y, R1.z, R2);

    return (R1 * 2.0f - 1.0f);

}

Face DecodeFaceData(EXPORTMeshlet InMeshlet, EXPORTFace InData)
{
    Face Result = (Face)0;

    Result.v0 = InMeshlet.VertexOffset + (InData.v012 & 0x3ff);
    Result.v1 = InMeshlet.VertexOffset + ((InData.v012 >> 10) & 0x3ff);
    Result.v2 = InMeshlet.VertexOffset + ((InData.v012 >> 20) & 0x3ff);

    uint adjFace01 = InData.edge01 & 0x3fffffff;
    Result.adjFace01 = (int)adjFace01 - 1;
    Result.e01h = ((InData.edge01 >> 30) & 0x1) == 1 ? true : false;
    Result.e01u = ((InData.edge01 >> 31) & 0x1) == 1 ? true : false;

    uint adjFace12 = InData.edge12 & 0x3fffffff;
    Result.adjFace12 = (int)adjFace12 - 1;
    Result.e12h = ((InData.edge12 >> 30) & 0x1) == 1 ? true : false;
    Result.e12u = ((InData.edge12 >> 31) & 0x1) == 1 ? true : false;

    uint adjFace20 = InData.edge20 & 0x3fffffff;
    Result.adjFace20 = (int)adjFace20 - 1;
    Result.e20h = ((InData.edge20 >> 30) & 0x1) == 1 ? true : false;
    Result.e20u = ((InData.edge20 >> 31) & 0x1) == 1 ? true : false;

   UnpackUint3ToNormals(InData.PackNormalData0, Result.Normal, Result.V0Normal);
    UnpackUint3ToNormals(InData.PackNormalData1, Result.V1Normal, Result.V2Normal);


    return Result;
}
