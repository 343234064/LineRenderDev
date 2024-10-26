Shader "Unlit/TestAdjacencyShade"
{
    Properties
    {
        _BaseColor("BaseColor", Color) = (1,1,1,1)
        _BackColor("BackColor", Color) = (0,0,0,1)
    }
    SubShader
    {
        Cull off
        Tags { "RenderType"="Opaque" }
        LOD 100

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            // make fog work
            #pragma multi_compile_fog

            #include "UnityCG.cginc"

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
                float3 normal : NORMAL;
            };

            struct v2f
            {
                float4 vertex : SV_POSITION;
                float2 uv : TEXCOORD0;
                float3 normal : TEXCOORD1;
                float3 worldpos : TEXCOORD2;

                UNITY_FOG_COORDS(3)
            };

            half4 _BaseColor;
            half4 _BackColor;

            float4 CustomWorldCameraPosition;

            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = UnityObjectToClipPos(v.vertex);
                o.uv = v.uv;

                o.normal = UnityObjectToWorldNormal(v.normal);
                o.worldpos = mul(unity_ObjectToWorld, v.vertex).xyz;

                UNITY_TRANSFER_FOG(o, o.vertex);
                return o;
            }

            fixed4 frag (v2f i) : SV_Target
            {
     
                fixed4 col = _BaseColor;
                half ndl = dot(_WorldSpaceLightPos0.xyz, normalize(i.normal));
                col = _BaseColor * ndl + _BackColor * (1.0h-ndl);
                
                float3 view = normalize(CustomWorldCameraPosition.xyz - i.worldpos);
                float f = dot(normalize(i.normal), view) > 0.0f ? 1.0f : 0.0f;
                col = lerp(col * fixed4(1.0, 0.1, 0.1, 1.0) , col, f);

                // apply fog
                UNITY_APPLY_FOG(i.fogCoord, col);
                return col;
            }
            ENDCG
        }

        Pass {
            Name "ShadowCaster"
            Tags { "LightMode" = "ShadowCaster" }

            ZWrite On ZTest LEqual

            CGPROGRAM
            #pragma target 2.0

            #pragma skip_variants SHADOWS_SOFT
            #pragma multi_compile_shadowcaster

            #pragma vertex vertShadowCaster
            #pragma fragment fragShadowCaster

            #include "UnityStandardShadow.cginc"

            ENDCG
        }
    }
}
