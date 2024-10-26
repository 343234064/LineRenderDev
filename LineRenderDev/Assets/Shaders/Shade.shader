// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'



Shader "LineRender/LineShader"{
	Properties{
		[HDR] TintColor("Tint", Color) = (1, 1, 1, 1)
		WorldPositionOffset("World Position Offset", Vector) = (0.0, 0.0, 0.0, 0.0)
		RandomOffset("RandomOffset", float) = 0.0
	}

		SubShader{
		Tags{ "RenderType" = "Opaque" "Queue" = "Transparent" }

		Pass{
			//ZTest Less
			//ZWrite On

			CGPROGRAM
			#include "UnityCG.cginc"
			
			#pragma vertex vert
			#pragma fragment frag

			fixed4 TintColor;
			float4 WorldPositionOffset;
			float MeshletLayer1Strength;
			float MeshletLayer2Strength;
			float RandomOffset;

			//UNITY_DECLARE_DEPTH_TEXTURE(_CameraDepthTexture);

			#include "Common.cginc"


			StructuredBuffer<Segment> SegmentList;


			struct Output
			{
				float4 position : SV_POSITION;
				float2 uv : TEXCOORD0;
				float curvature : TEXCOORD1;

				float4 color : TEXCOORD2;
			};



			Output vert(uint vertex_id: SV_VertexID, uint instance_id : SV_InstanceID)
			{
				/*
				const uint index[2][3] = { 0, 1, 2, 1, 3, 2 };
				uint instance_select = instance_id % 2 == 0;
				uint instanceId = instance_select ? instance_id * 0.5 : (instance_id - 1) * 0.5;
				uint vertexId =  index[instance_select][vertex_id];
				
				LineMesh Line = Lines[instanceId];
				float4 position = ReverseUnifyNDCPosition(float4(Line.Position[vertexId], 1.0f, 1.0f)); // w = 1.0f to turn off perspective divide
				float2 uv = Line.UV[vertexId];
				float curvature = Line.Curvature[vertexId < 2 ? 0 : 1];

				Output output = (Output)0;
				output.position = position;
				output.uv = uv;
				output.curvature = curvature;
				
				output.color = float4(1.0, 1.0, 1.0, 1.0);
				*/

				Segment CurrentLine = SegmentList[instance_id];
				float ran = Hash31((float)instance_id * 10).x;
				float4 Position = ReverseUnifyNDCPosition(float4(CurrentLine.NDCPosition[vertex_id].xy + ran*RandomOffset, 1.0f, 1.0f)); 
				//float4 Position = UnityObjectToClipPos(float4(CurrentLine.NDCPosition[vertex_id].xyz, 1.0f));

				Output output = (Output)0;
				output.position = Position;

				output.color = CurrentLine.Color;// float4(1.0, 1.0, 0.0, 1.0);
				//output.color.rgb = Hash31((float)instance_id*10);
				// 
				// 
				//if (Line.BackFacing == 1)
				//	output.color = float4(1.0, 1.0, 0.0, 1.0);
				//if (Line.Visibility == 0)
				//	output.color *= float4(0.5, 0.5, 0.0, 1.0);

				
				/*
				if (Line.LinkState[vertex_id] == 0) {
					AdjVertex V1 = Vertices[Line.VertexIndex[vertex_id]];
					output.color.rgb = lerp(float3(1.0, 1.0, 1.0), float3(1.0, 0.0, 0.0), V1.IsInMeshletBorder2);
				}
				else
				{
					output.color.rgb = float3(1.0, 1.0, 1.0);
				}*/
				
				
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
