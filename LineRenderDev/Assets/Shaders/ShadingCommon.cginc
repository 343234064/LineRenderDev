//////////////////////////
//Shading Common
// 
/////////////////////////


#pragma multi_compile __ ENABLE_DEBUG_VIEW
#include "Common.cginc"


struct LineVertex
{
	//In ndc space
	float4 Position;

	//Line type
	/*Use these enums :
		CONTOUR_FACE_TYPE,
		CONTOUR_EDGE_TYPE,
		CREASE_EDGE_TYPE,
		BORDER_EDGE_TYPE,
		CUSTOM_EDGE_TYPE,
	*/
	uint Type;

	//Is this line visible
	bool Visible;
	//Is this line backfacing
	bool BackFacing;

	//Normal of this vertex
	float2 Normal;

	//Id of this line
	float Id;
};












StructuredBuffer<LineMesh> LineMeshBuffer;
#if ENABLE_DEBUG_VIEW
float4x4 WorldViewProjectionForDebugCamera;
float4x4 InvWorldViewProjectionForRenderCamera;
#endif
float LineWidthScale;
float4 ScreenScaledResolution;


float4 GetExtractDirection(float2 P0, float2 P1, uint InstanceSelect, uint IndexSelect, float ExtractWidth, float CenterOffset)
{
	float ExtractWidthScaled = ExtractWidth * LineWidthScale;
	float InnerSideWidth = ExtractWidthScaled * (CenterOffset + 0.5f);
	float OuterSideWidth = ExtractWidthScaled * (0.5f - CenterOffset);

	float4 Normal = GetNormalOfLine(P0, P1);
	float ScreenScaledFactor = ScreenScaledResolution.x * ScreenScaledResolution.w;
	Normal.y *= ScreenScaledFactor;
	Normal.w *= ScreenScaledFactor;

	const uint Index[2][3] = { {0, 0, 1}, {1, 0, 1} };
	float4 Extracted[2];
	Extracted[0] = float4(Normal.xy * InnerSideWidth, Normal.xy);
	Extracted[1] = float4(Normal.zw * OuterSideWidth, Normal.zw);
	//Extracted[2] = Normal.xy * InnerSideWidth;
	//Extracted[3] = Normal.zw * OuterSideWidth;

	return Extracted[Index[InstanceSelect][IndexSelect]];
}


LineVertex PostProcessing(uint InInstanceId, uint InVertexId, float ExtractWidth, float CenterOffset)
{
	uint InstanceId = InInstanceId;
	uint VertexId = InVertexId;
	uint InstanceSelect = (InInstanceId % 2 == 0) ? 0 : 1;
	const uint Index[2][3] = { {0, 1, 0}, {0, 1, 1} };
	//const uint Index[2][3] = { {0, 2, 1}, {1, 2, 3} };

#if !ENABLE_DEBUG_VIEW
	InstanceId = (InstanceSelect == 0) ? InInstanceId * 0.5 : (InInstanceId - 1) * 0.5;
	VertexId = Index[InstanceSelect][InVertexId];
#endif
	LineMesh CurrentLine = LineMeshBuffer[InstanceId];

	float4 ExtractDirection = GetExtractDirection(CurrentLine.NDCPosition[0].xy, CurrentLine.NDCPosition[1].xy, InstanceSelect, InVertexId, ExtractWidth, CenterOffset);
	float2 CurrentNDCVertex = CurrentLine.NDCPosition[VertexId] + ExtractDirection.xy;

	float4 Position = float4(0.0f, 0.0f, 0.0f, 0.0f);
#if !ENABLE_DEBUG_VIEW

	Position = ReverseUnifyNDCPosition(float4(CurrentNDCVertex, 1.0f, 1.0f)); // w = 1.0f to turn off perspective divide

#else

	float4 DebugNDCPosition = ReverseUnifyNDCPosition(CurrentLine.NDCPositionForDebug[VertexId]);
	float4 DebugClipPosition = float4(DebugNDCPosition.xyz * DebugNDCPosition.w, DebugNDCPosition.w);
	float4 DebugLocalPosition = mul(InvWorldViewProjectionForRenderCamera, DebugClipPosition);
	Position = mul(WorldViewProjectionForDebugCamera, DebugLocalPosition);
#if REVERSED_Z
	Position.z = 1.0f;
#else
	Position.z = 0.0f;
#endif

#endif








	LineVertex NewVertex = (LineVertex)0;
	NewVertex.Position = Position;
	NewVertex.Type = GET_LINE_TYPE(CurrentLine.Type);
	NewVertex.Visible = GET_LINE_IS_VISIBLE(CurrentLine.Type);
	NewVertex.BackFacing = GET_LINE_IS_BACKFACING(CurrentLine.Type);
	NewVertex.Normal = ExtractDirection.zw;
	NewVertex.Id = CurrentLine.PixelPosition[0] + CurrentLine.PixelPosition[1];// GET_ANCHOR_INDEX(CurrentLine.Anchor[0]) + GET_ANCHOR_INDEX(CurrentLine.Anchor[1]);

	return NewVertex;
}



float3 DebugColor(LineVertex InVertex)
{
	float3 Color = float3(0.0, 0.0, 0.0);
#if ENABLE_DEBUG_VIEW

	uint Type = InVertex.Type;
	bool Visible = InVertex.Visible;
	bool BackFacing = InVertex.BackFacing;
	if (!Visible)
	{
		Color.rgb = float3(1.0, 1.0, 0.0);
		if (BackFacing)
		{
			Color.rgb = float3(1.0, 1.0, 0.5);
		}
	}
	else {
		if (Type == CONTOUR_EDGE_TYPE || Type == CONTOUR_FACE_TYPE) Color.rgb = float3(1.0, 0.0, 0.0);
		if (Type == CREASE_EDGE_TYPE) Color.rgb = float3(0.0, 1.0, 0.0);

	}
	if (Type == DEBUG_TYPE) Color.rgb = float3(0.2, 0.2, 0.1);

#else

	//float TestAttr = (VertexId == 0) ? CurrentLine.GroupId[0] : CurrentLine.GroupId[1];
	//TestAttr = CurrentLine.GroupId[1];
	//Color.rgb = Hash31(TestAttr); saturate(TestAttr);
	//Color.rgb = CurrentLine.GroupId.x==0?1:0;

#endif

	return Color;

}