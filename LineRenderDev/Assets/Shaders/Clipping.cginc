//////////////////////////
//Clipping
// 
/////////////////////////

#ifndef ENABLE_DEBUG_CLIPPING
#define ENABLE_DEBUG_CLIPPING 0
#endif

uniform float4x4 WorldViewProjection;
uniform float4x4 WorldViewProjectionForClipping;

#define CLIPLEFT(p) -p.w
#define CLIPRIGHT(p) p.w
#define CLIPTOP(p) p.w
#define CLIPBOTTOM(p) -p.w
#if NEGATIVE_CLIP_Z_VALUE
#define CLIPFAR(p) p.w  //ReverseZ do not matter
#define CLIPNEAR(p) -p.w
#else
#define CLIPFAR(p) p.w  //ReverseZ do not matter
#define CLIPNEAR(p) 0.0f
#endif

inline bool InX(float4 P) { return P.x >= CLIPLEFT(P) && P.x <= CLIPRIGHT(P); }
inline bool InY(float4 P) { return P.y >= CLIPBOTTOM(P) && P.y <= CLIPTOP(P); }
inline bool InZ(float4 P) { return P.z >= CLIPNEAR(P) && P.z <= CLIPFAR(P); }


int IsIn(float4 ClipPoint1, float4 ClipPoint2)
{
    bool P1X = InX(ClipPoint1);
    bool P1Y = InY(ClipPoint1);
    bool P1Z = InZ(ClipPoint1);
    bool P1In = P1X && P1Y && P1Z;

    bool P2X = InX(ClipPoint2);
    bool P2Y = InY(ClipPoint2);
    bool P2Z = InZ(ClipPoint2);
    bool P2In = P2X && P2Y && P2Z;

    int State = P1In && P2In ? 0 : -1;
    State = P1In && !P2In ? 1 : State;
    State = !P1In && P2In ? 2 : State;

    return State;
}

float IntersectWithXPlane(float ClipPlaneP1, float ClipPlaneP2, float4 P1, float4 P2, uint InterectVecIndex, inout uint InterectVec)
{
    float KFactor = P2.x - P1.x - ClipPlaneP2 + ClipPlaneP1;
    float T = (KFactor == 0.0f) ? -1.0f : ((ClipPlaneP1 - P1.x) / KFactor);
    uint Intersect = 0;
    if (T >= 0.0f && T <= 1.0f)
    {
        float4 IntersectP = P1 + T * (P2 - P1);
        Intersect = (InY(IntersectP) && InZ(IntersectP)) ? 1 : 0;
    }
    InterectVec = InterectVec | (Intersect << InterectVecIndex);

    return T;
}

float IntersectWithYPlane(float ClipPlaneP1, float ClipPlaneP2, float4 P1, float4 P2, uint InterectVecIndex, inout uint InterectVec)
{
    float KFactor = P2.y - P1.y - ClipPlaneP2 + ClipPlaneP1;
    float T = (KFactor == 0.0f) ? -1.0f : ((ClipPlaneP1 - P1.y) / KFactor);
    uint Intersect = 0;
    if (T >= 0.0f && T <= 1.0f)
    {
        float4 IntersectP = P1 + T * (P2 - P1);
        Intersect = (InX(IntersectP) && InZ(IntersectP)) ? 1 : 0;
    }
    InterectVec = InterectVec | (Intersect << InterectVecIndex);

    return T;
}

float IntersectWithZPlane(float ClipPlaneP1, float ClipPlaneP2, float4 P1, float4 P2, uint InterectVecIndex, inout uint InterectVec)
{
    float KFactor = P2.z - P1.z - ClipPlaneP2 + ClipPlaneP1;
    float T = (KFactor == 0.0f) ? -1.0f : ((ClipPlaneP1 - P1.z) / KFactor);
    uint Intersect = 0;
    if (T >= 0.0f && T <= 1.0f)
    {
        float4 IntersectP = P1 + T * (P2 - P1);
        Intersect = (InX(IntersectP) && InY(IntersectP)) ? 1 : 0;
    }
    InterectVec = InterectVec | (Intersect << InterectVecIndex);

    return T;
}

float3 PreClipping(float4 ClipPosition1, float4 ClipPosition2)
{
    float4 ClipPos1 = ClipPosition1;
    float4 ClipPos2 = ClipPosition2;

    if (ClipPos1.w == 0.0f || ClipPos2.w == 0.0f)
        return float3(-1.0f, 0.0f, 0.0f);

    float3 Result = float3(1.0f, 0.0f, 1.0f);
    int State = IsIn(ClipPos1, ClipPos2);
    if (State != 0)
    {
        float IntersectT[6] = { 0.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f };
        uint InterectVec = 0;
        IntersectT[0] = IntersectWithXPlane(CLIPLEFT(ClipPos1), CLIPLEFT(ClipPos2), ClipPos1, ClipPos2, 0, InterectVec);
        IntersectT[1] = IntersectWithXPlane(CLIPRIGHT(ClipPos1), CLIPRIGHT(ClipPos2), ClipPos1, ClipPos2, 1, InterectVec);
        IntersectT[2] = IntersectWithYPlane(CLIPTOP(ClipPos1), CLIPTOP(ClipPos2), ClipPos1, ClipPos2, 2, InterectVec);
        IntersectT[3] = IntersectWithYPlane(CLIPBOTTOM(ClipPos1), CLIPBOTTOM(ClipPos2), ClipPos1, ClipPos2, 3, InterectVec);
        IntersectT[4] = IntersectWithZPlane(CLIPNEAR(ClipPos1), CLIPNEAR(ClipPos2), ClipPos1, ClipPos2, 4, InterectVec);
        IntersectT[5] = IntersectWithZPlane(CLIPFAR(ClipPos1), CLIPFAR(ClipPos2), ClipPos1, ClipPos2, 5, InterectVec);

        uint IntersectIndex1 = firstbitlow(InterectVec);
        uint IntersectIndex2 = firstbithigh(InterectVec);
        if (State == 1)
        {
            Result.xz = IntersectIndex1 < 6 ? float2(1.0f, IntersectT[IntersectIndex1]) : float2(-1.0f, Result.z);
        }
        else if (State == 2)
        {
            Result.xy = IntersectIndex1 < 6 ? float2(1.0f, IntersectT[IntersectIndex1]) : float2(-1.0f, Result.y);
        }
        else
        {
            Result.xyz = (IntersectIndex1 < 6 && IntersectIndex2 < 6) ? float3(1.0f, IntersectT[IntersectIndex1], IntersectT[IntersectIndex2]) : float3(-1.0f, 0.0f, 0.0f);
            float T1 = Result.y;
            float T2 = Result.z;
            Result.y = min(T1, T2);
            Result.z = max(T1, T2);
        }

    }

    return Result;
}

inline float4 ClampPosition(float4 P)
{
    //return float4(min(CLIPRIGHT(P), max(CLIPLEFT(P), P.x)), min(CLIPTOP(P), max(CLIPBOTTOM(P), P.y)), min(CLIPFAR(P), max(CLIPNEAR(P), P.z)), P.w);
    return P;
}


float4 CalculateClipSpacePosition(float3 LocalPosition)
{
    float4 Transformed = mul(WorldViewProjection, float4(LocalPosition, 1.0));
    //Transformed.xyz = Transformed.xyz / Transformed.w;

    return Transformed;
}

float4 CalculateClipSpacePositionForClipping(float3 LocalPosition)
{
    float4 Transformed = mul(WorldViewProjectionForClipping, float4(LocalPosition, 1.0));

    return Transformed;
}

uint4 CheckVertexIndex(uint2 VertexIndex, float3 ClipState)
{
    return uint4(VertexIndex, ClipState.y <= 0.0000001f ? 0 : 1, ClipState.z >= 0.9999999f ? 0 : 1);
}


void ToSegmentLine(float3 V0, float3 V1, inout Segment OutSegment)
{
    float4 Pos0BeforeClip = CalculateClipSpacePosition(V0);
    float4 Pos1BeforeClip = CalculateClipSpacePosition(V1);

    float4 ClipPos0BeforeClip = Pos0BeforeClip;
    float4 ClipPos1BeforeClip = Pos1BeforeClip;
#if ENABLE_DEBUG_CLIPPING
    ClipPos0BeforeClip = CalculateClipSpacePositionForClipping(V0);
    ClipPos1BeforeClip = CalculateClipSpacePositionForClipping(V1);
#endif

    float3 ClipState = PreClipping(ClipPos0BeforeClip, ClipPos1BeforeClip);
    float4 ClipPos0 = ClampPosition(float4(Pos0BeforeClip + ClipState.y * (Pos1BeforeClip - Pos0BeforeClip)));
    float4 ClipPos1 = ClampPosition(float4(Pos0BeforeClip + ClipState.z * (Pos1BeforeClip - Pos0BeforeClip)));

    float4 NDCPos0 = CalculateNDCPosition(ClipPos0);
    float4 NDCPos1 = CalculateNDCPosition(ClipPos1);
    NDCPos0 = UnifyNDCPosition(NDCPos0);
    NDCPos1 = UnifyNDCPosition(NDCPos1);

    OutSegment.NDCPosition[0] = NDCPos0;
    OutSegment.NDCPosition[1] = NDCPos1;

}
