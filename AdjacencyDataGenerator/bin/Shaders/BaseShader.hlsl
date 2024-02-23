#ifndef DRAW_LINE
#define DRAW_LINE 0
#endif

cbuffer ConstantBuffer : register(b0)
{
	float4x4 WorldViewProjection;
};


struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 color : COLOR0;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float3 normal : TEXCOORD0;
	float3 color : TEXCOORD1;
};


VSOutput VSMain(VSInput input)
{
	VSOutput output = (VSOutput)0;

	float4 positionLocal = float4(input.position, 1.0f);
	output.normal = normalize(input.normal);
#if DRAW_LINE
	positionLocal.xyz = positionLocal.xyz + output.normal.xyz * 0.001f;
#endif
	output.position = mul(positionLocal, WorldViewProjection); 
	output.color = input.color;
	
	return output;
}

float4 PSMain(VSOutput input) : SV_TARGET0
{
	float3 color = float3(0.0f,0.0f,0.0f);
#if !DRAW_LINE
	float3 NdL = dot(input.normal, float3(-1.0, 1.0, 1.0)) * 0.5f + 0.5f;
	color = lerp(float3(0.2, 0.2, 0.3), float3(0.3, 0.3, 0.8), NdL);
#else
	color = float3(1.0f,1.0f,1.0f);
#endif

	return float4(color,1.0);
}
