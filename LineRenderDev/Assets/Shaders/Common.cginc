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

/*
* Dispatch arguments
* 
*/
#define EXTRACT_PASS_PER_THREAD_ELEMENT_NUM 256
//
#define CONTOURIZE_PASS_PER_THREAD_ELEMENT_NUM 256
// Must be power of 2
// Slice pass max handle num : BUCKET_ELEMENT_NUM * BUCKET_ELEMENT_NUM
#define SLICE_PASS_PER_THREAD_ELEMENT_NUM 1024
#define SLICE_PIXEL_SIZE 64
//
#define VISIBILITY_PASS_PER_THREAD_ELEMENT_NUM 64
#define VISIBILITY_CACHE_NUM 2 // 64/32
//
#define GENERATE_PASS_PER_THREAD_ELEMENT_NUM 256
//
#define CHAINNING_PASS_PER_THREAD_ELEMENT_NUM 256


/*
* ArgBuffer Offset
* 
*/
#define TOTAL_ARGS_COUNT 28
#define INDIRECT_DRAW_START 0
#define INDIRECT_DRAW_VERTEX_COUNT  INDIRECT_DRAW_START
#define INDIRECT_DRAW_TRIANGLE_COUNT  INDIRECT_DRAW_START+1
#define CONTOURIZE_PASS_START 4
#define CONTOURIZE_PASS_DISPATCH_COUNT  CONTOURIZE_PASS_START
#define CONTOURIZE_PASS_CONTOUR_COUNT  CONTOURIZE_PASS_START+3
#define SLICE_PASS_START 8
#define SLICE_PASS_DISPATCH_COUNT  SLICE_PASS_START
#define SLICE_PASS_CONTOUR_COUNT  SLICE_PASS_START+3
#define VISIBILITY_PASS_START 12
#define VISIBILITY_PASS_DISPATCH_COUNT  VISIBILITY_PASS_START
#define VISIBILITY_PASS_SLICE_COUNT  VISIBILITY_PASS_START
#define GENERATE_PASS_START 16
#define GENERATE_PASS_DISPATCH_COUNT  GENERATE_PASS_START
#define GENERATE_PASS_SEGMENT_COUNT  GENERATE_PASS_START+3
#define CHAINNING_PASS_START 20
#define CHAINNING_PASS_DISPATCH_COUNT  CHAINNING_PASS_START
#define CHAINNING_PASS_LINEHEAD_COUNT  CHAINNING_PASS_START+3
#define CHAINNING_PASS_LAYER1_OFFSET 0
#define CHAINNING_PASS_LAYER2_OFFSET 4
#define CHAINNING_PASS_LAYER3_OFFSET 8



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

inline bool ZTest(float PositionDepth, float DepthScale, float SceneDepth)
{
    const float DepthOffset = 0.0005f * DepthScale;
#if REVERSED_Z
    return ((PositionDepth + DepthOffset) > SceneDepth) ? true : false;
#else
    return ((PositionDepth - DepthOffset) < SceneDepth) ? true : false;
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

Face DecodeFaceData(EXPORTFace InData)
{
    Face Result = (Face)0;

    Result.v0Offset = (InData.v012 & 0x3ff);
    Result.v1Offset = ((InData.v012 >> 10) & 0x3ff);
    Result.v2Offset = ((InData.v012 >> 20) & 0x3ff);

    uint adjFace01 = InData.edge01 & 0x0fffffff;
    Result.adjFace01 = (int)adjFace01 - 1;
    Result.e01h = ((InData.edge01 >> 30) & 0x1) == 1 ? true : false;
    Result.e01u = ((InData.edge01 >> 31) & 0x1) == 1 ? true : false;

    uint adjFace12 = InData.edge12 & 0x0fffffff;
    Result.adjFace12 = (int)adjFace12 - 1;
    Result.e12h = ((InData.edge12 >> 30) & 0x1) == 1 ? true : false;
    Result.e12u = ((InData.edge12 >> 31) & 0x1) == 1 ? true : false;

    uint adjFace20 = InData.edge20 & 0x0fffffff;
    Result.adjFace20 = (int)adjFace20 - 1;
    Result.e20h = ((InData.edge20 >> 30) & 0x1) == 1 ? true : false;
    Result.e20u = ((InData.edge20 >> 31) & 0x1) == 1 ? true : false;

    Result.edge01 = InData.UniqueIndex01;
    Result.edge12 = InData.UniqueIndex12;
    Result.edge20 = InData.UniqueIndex20;

    UnpackUint3ToNormals(InData.PackNormalData0, Result.Normal, Result.V0Normal);
    UnpackUint3ToNormals(InData.PackNormalData1, Result.V1Normal, Result.V2Normal);

    Result.MeshletData = InData.MeshletData;

    return Result;
}


float3 ComputeNormal(float3 p1, float3 p2, float3 p3)
{
    float3 U = p2 - p1;
    float3 V = p3 - p1;

    float3 Normal = float3(0.0f, 0.0f, 0.0f);
    /*
    Normal.x = U.y * V.z - U.z * V.y;
    Normal.y = U.z * V.x - U.x * V.z;
    Normal.z = U.x * V.y - U.y * V.x;
    */
    Normal = cross(U, V);
    // No need to normalize
    Normal = normalize(Normal);

    return Normal;
}



uint2 CalculatePixelLength(float2 UnifyNDCPosition1, float2 UnifyNDCPosition2, float2 ScreenResolution)
{
    float ScreenWidth = ScreenResolution.x;
    float ScreenHeight = ScreenResolution.y;

    float2 PixelPos1 = float2(UnifyNDCPosition1.x * ScreenWidth, UnifyNDCPosition1.y * ScreenHeight);
    float2 PixelPos2 = float2(UnifyNDCPosition2.x * ScreenWidth, UnifyNDCPosition2.y * ScreenHeight);

    float PixelLength = ceil(length(PixelPos2 - PixelPos1));
    float ShrinkPixelLength = PixelLength > 2.0 ? (1.0 + floor(PixelLength * 0.5)) : PixelLength;

    return uint2((uint)PixelLength, (uint)(ShrinkPixelLength));
}



float4 GetNormalOfLine(float2 a, float2 b)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;

    return float4(normalize(float2(-dy, dx)), normalize(float2(dy, -dx)));
}
