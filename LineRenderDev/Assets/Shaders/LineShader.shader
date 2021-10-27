// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "LineRender/LineShader"{
	//show values to edit in inspector
	Properties{
		[HDR] _Color("Tint", Color) = (0, 0, 0, 1)
	}

		SubShader{
		//the material is completely non-transparent and is rendered at the same time as the other opaque geometry
		Tags{ "RenderType" = "Opaque" "Queue" = "Geometry" }

		Pass{
			CGPROGRAM

			//include useful shader functions
			#include "UnityCG.cginc"

			//define vertex and fragment shader functions
			#pragma vertex vert
			#pragma fragment frag


			fixed4 _Color;
			float4x4 _ObjectWorldMatrix;

			//buffers
			StructuredBuffer<uint2> LinesIndex;
			StructuredBuffer<float3> Positions;

			//the vertex shader function
			float4 vert(uint vertex_id: SV_VertexID, uint instance_id : SV_InstanceID) : SV_POSITION
			{
				//get vertex position
				uint2 edgeIndex = LinesIndex[instance_id];
				uint positionIndex = edgeIndex[vertex_id];
				float3 localposition = Positions[positionIndex];

				float4 worldposition = mul(_ObjectWorldMatrix, float4(localposition, 1));

				float4 position = UnityWorldToClipPos(worldposition);
				return position;
			}

			//the fragment shader function
			fixed4 frag() : SV_TARGET{
				//return the final color to be drawn on screen
				return _Color;
			}

			ENDCG
			}
		}
		Fallback "VertexLit"
}
