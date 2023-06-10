// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "LineRender/LineShader"{
	Properties{
		[HDR] TintColor("Tint", Color) = (0, 0, 0, 1)
		WorldPositionOffset("World Position Offset", Vector) = (0.0, 0.0, 0.0, 0.0)
	}

		SubShader{
		Tags{ "RenderType" = "Opaque" "Queue" = "Transparent" }

		Pass{
			ZTest Always
			ZWrite Off

			CGPROGRAM
			#include "UnityCG.cginc"
			
			#pragma vertex vert
			#pragma fragment frag


			fixed4 TintColor;
			float4 WorldPositionOffset;

			UNITY_DECLARE_DEPTH_TEXTURE(_CameraDepthTexture);

			#include "Common.cginc"

			StructuredBuffer<PlainLine> Lines;
			//StructuredBuffer<float3> Positions;

			struct Output
			{
				float4 position : SV_POSITION;
				float4 screenpos : TEXCOORD0;
				float4 color : TEXCOORD1;
			};

			float Hash(float2 p)
			{
				p = 50.0 * frac(p * 0.3183099 + float2(0.71, 0.113));
				return -1.0 + 2.0 * frac(p.x * p.y * (p.x + p.y));
			}

			Output vert(uint vertex_id: SV_VertexID, uint instance_id : SV_InstanceID)
			{
				//uint2 edgeIndex = LinesIndex[instance_id];
				//uint positionIndex = edgeIndex[vertex_id];
				//float3 localposition = Positions[positionIndex];
				PlainLine Line = Lines[instance_id];
				//float3 localposition = Line.LocalPosition[vertex_id];

				//float4 worldposition = mul(ObjectWorldMatrix, float4(localposition, 1));
				//worldposition.xyz += WorldPositionOffset.xyz;
				float4 position = ReverseUnifyNDCPosition(float4(Line.NDCPosition[vertex_id].xy, 1.0f, 1.0f)); // 1.0f to turn off perspective divide
				//position = UnityWorldToClipPos(worldposition);

				Output output = (Output)0;
				output.position = position;

				
				output.color = float4(1.0, 1.0, 1.0, 1.0);
				if (Line.BackFacing < 1)
					output.color *= float4(1.0, 1.0, 1.0, 1.0);
				else
					output.color *= float4(1.0, 0.0, 1.0, 1.0);
				float r = saturate(Hash(float2(Line.SliceIndex, 2.0 / float(Line.SliceIndex))));
				float g = saturate(Hash(float2(2.0 / float(Line.SliceIndex), Line.SliceIndex)));
				float b = saturate(Hash(float2(5.0 / float(Line.SliceIndex), Line.SliceIndex)));
				//if(Line.SliceIndex ==18)
				//	output.color = float4(1., 0., 0., 1.0f);

				//if (Line.SliceIndex == 63)
				//	output.color = float4(0., 1., 1., 1.0f);
				//output.color = float4(r,g,b, 1.0f);
				//output.color = Line.SliceIndex == 1 ? float4(1.0, 0.0, 0.0, 1.0) : float4(0.0, 1.0, 0.0, 1.0);
				return output;
			}

			fixed4 frag(Output input) : SV_TARGET{

				//float depthTexValue = SAMPLE_DEPTH_TEXTURE_PROJ(_CameraDepthTexture, UNITY_PROJ_COORD(input.screenpos));
				fixed4 finalColor = fixed4(1, 1, 1, 1);

				//if (((input.screenpos.z / input.screenpos.w)) >= depthTexValue)
					finalColor.rgb = TintColor.rgb * input.color.rgb;
	
				return finalColor;
			}

			ENDCG
			}
		}
		Fallback "VertexLit"
}
