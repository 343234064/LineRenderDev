// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'



Shader "LineRender/LineShader"{
	Properties{
		[HDR] TintColor("Tint", Color) = (1, 1, 1, 1)
		LineExtractWidth("Width", float) = 1.0
		LineCenterOffset("Center Offset", float) = 0.0
		PositionOffset("Position Offset", Vector) = (0.0, 0.0, 0.0, 0.0)
		RandomOffset("RandomOffset", float) = 0.0
	}

		SubShader{
		Tags{ "RenderType" = "Opaque" "Queue" = "Transparent" }

		Pass{
			//Cull Off
			ZTest Off
			ZWrite Off

			CGPROGRAM
			#include "UnityCG.cginc"
			
			#pragma vertex vert
			#pragma fragment frag

			fixed4 TintColor;
			float LineExtractWidth;
			float LineCenterOffset;
			float4 PositionOffset;
			float RandomOffset;

			
			#include "ShadingCommon.cginc"


			struct Output
			{
				float4 position : SV_POSITION;
				float2 uv : TEXCOORD0;
				float3 color : TEXCOORD1;
			};


			Output vert(uint vertex_id: SV_VertexID, uint instance_id : SV_InstanceID)
			{
				LineVertex InVertex = PostProcessing(instance_id, vertex_id, LineExtractWidth, LineCenterOffset);

				Output output = (Output)0;

				float RanID = InVertex.Id;
				float Ran = Hash31(RanID).x;
				InVertex.Position.xy += (Ran * RandomOffset + PositionOffset.xy);

				output.position = InVertex.Position;

				float3 Color = float3(1.0, 1.0, 1.0);
				Color = DebugColor(InVertex);
				
				output.color = Color;

				return output;
			}

			fixed4 frag(Output input) : SV_TARGET
			{

				fixed4 finalColor = fixed4(1, 1, 1, 1);
				finalColor.rgb = input.color.rgb;// TintColor.rgb* input.color.rgb;
	
				return finalColor;
			}

			ENDCG
			}
		}
		Fallback "VertexLit"
}
