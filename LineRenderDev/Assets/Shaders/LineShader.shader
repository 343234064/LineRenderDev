// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "LineRender/LineShader"{
	Properties{
		[HDR] _Color("Tint", Color) = (0, 0, 0, 1)
		_WorldPositionOffset("World Position Offset", Vector) = (0.0, 0.0, 0.0, 0.0)
	}

		SubShader{
		Tags{ "RenderType" = "Opaque" "Queue" = "Geometry" }

		Pass{
			ZTest Always

			CGPROGRAM
			#include "UnityCG.cginc"
			
			#pragma vertex vert
			#pragma fragment frag


			fixed4 _Color;
			float4 _WorldPositionOffset;
			float4x4 _ObjectWorldMatrix;

			StructuredBuffer<uint2> LinesIndex;
			StructuredBuffer<float3> Positions;

			float4 vert(uint vertex_id: SV_VertexID, uint instance_id : SV_InstanceID) : SV_POSITION
			{
				uint2 edgeIndex = LinesIndex[instance_id];
				uint positionIndex = edgeIndex[vertex_id];
				float3 localposition = Positions[positionIndex];

				float4 worldposition = mul(_ObjectWorldMatrix, float4(localposition, 1));
				worldposition.xyz += _WorldPositionOffset.xyz;
				float4 position = UnityWorldToClipPos(worldposition);
				return position;
			}

			fixed4 frag() : SV_TARGET{
				return _Color;
			}

			ENDCG
			}
		}
		Fallback "VertexLit"
}
