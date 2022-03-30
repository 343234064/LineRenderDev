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

			UNITY_DECLARE_DEPTH_TEXTURE(_CameraDepthTexture);

			struct LineSegment
			{
				float3 point3d[2];
			};

			StructuredBuffer<LineSegment> Lines;
			//StructuredBuffer<float3> Positions;

			struct Output
			{
				float4 position : SV_POSITION;
				float4 screenpos : TEXCOORD0;
			};

			inline float4 ComputeNonStereoScreenPos1(float4 pos) {
				float4 o = pos * 0.5f;
				o.xy = float2(o.x, o.y * _ProjectionParams.x) + o.w;
				o.zw = pos.zw;
				return o;
			}

			Output vert(uint vertex_id: SV_VertexID, uint instance_id : SV_InstanceID)
			{
				//uint2 edgeIndex = LinesIndex[instance_id];
				//uint positionIndex = edgeIndex[vertex_id];
				//float3 localposition = Positions[positionIndex];
				LineSegment Line = Lines[instance_id];
				float3 localposition = Line.point3d[vertex_id];

				float4 worldposition = mul(_ObjectWorldMatrix, float4(localposition, 1));
				worldposition.xyz += _WorldPositionOffset.xyz;
				float4 position = UnityWorldToClipPos(worldposition);

				Output output = (Output)0;
				output.position = position;
				output.screenpos = ComputeNonStereoScreenPos1(position);
				return output;
			}

			fixed4 frag(Output input) : SV_TARGET{

				//float depthTexValue = SAMPLE_DEPTH_TEXTURE_PROJ(_CameraDepthTexture, UNITY_PROJ_COORD(input.screenpos));
				fixed4 finalColor = fixed4(0, 0, 0, 1);

				//if (((input.screenpos.z / input.screenpos.w)) >= depthTexValue)
					finalColor = _Color;

				return finalColor;
			}

			ENDCG
			}
		}
		Fallback "VertexLit"
}
