#ifndef DRAW_WIREFRAME
#define DRAW_WIREFRAME 0
#endif
#ifndef DRAW_NORMAL
#define DRAW_NORMAL 0
#endif

cbuffer ConstantBuffer : register(b0)
{
	float4x4 WorldViewProjection;
};

cbuffer ParametersBuffer : register(b1)
{
	int DisplayMode;
	float DisplayTransparent0;
	float DisplayTransparent1;
	float DisplayTransparent2;
	float DisplayTransparent3;
};



struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float4 color : COLOR0;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float3 normal : TEXCOORD0;
	float3 color : TEXCOORD1;
};


float3 Hash31(float p)
{
   float3 p3 = frac(float3(p, p, p) * float3(0.1031f, 0.1030f, 0.0973f));
   p3 += dot(p3, p3.yzx + 33.33f);
   return frac((p3.xxy + p3.yzz) * p3.zyx);
}

VSOutput VSMain(VSInput input)
{
	VSOutput output = (VSOutput)0;

	float4 positionLocal = float4(input.position, 1.0f);
	output.normal = normalize(input.normal);
#if DRAW_WIREFRAME
	positionLocal.xyz = positionLocal.xyz + output.normal.xyz * 0.0001f;
#endif
	output.position = mul(positionLocal, WorldViewProjection); 

	output.color = float3(0.0f,0.0f,0.0f);
#if !DRAW_WIREFRAME && !DRAW_NORMAL
	float3 NdL = dot(output.normal, float3(-1.0, 1.0, 1.0)) * 0.5f + 0.5f;
	output.color = lerp(float3(0.2f, 0.2f, 0.3f), float3(0.3f, 0.3f, 0.8f), NdL);

	if(DisplayMode == 1){
		float3 layer1Color = (input.color.x < 0.0f) ? float3(0.0f,0.0f,0.0f) : Hash31(input.color.x+1);
		output.color = lerp(output.color, layer1Color, DisplayTransparent0);
		float3 layer2Color = (input.color.y < 0.0f) ? float3(0.0f,0.0f,0.0f) : Hash31(input.color.y+1);
		output.color = lerp(output.color, layer2Color, DisplayTransparent1);
		float3 layer3Color = (input.color.z < 0.0f) ? float3(0.0f, 0.0f, 0.0f) : Hash31(input.color.z + 1);
		output.color = lerp(output.color, layer3Color, DisplayTransparent2);
	}

#endif

	return output;
}

float4 PSMain(VSOutput input) : SV_TARGET0
{
	float4 color = float4(0.0f,0.0f,0.0f,0.0f);
#if DRAW_WIREFRAME
	color = float4(0.8f,0.8f,0.9f, 1.0f);
#elif DRAW_NORMAL
	color = float4(0.2f,0.8f,0.2f, 1.0f);
#else
	color = float4(input.color,1.0f);
#endif

	return color;
}
