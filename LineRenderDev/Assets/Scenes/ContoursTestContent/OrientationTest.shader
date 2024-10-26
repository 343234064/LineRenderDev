Shader "Unlit/OrientationTest"
{
    Properties
    {
        [HDR] FrontFaceColor("Front Face Color", Color) = (1, 1, 1, 1)
        [HDR] BackFaceColor("Back Face Color", Color) = (1, 1, 1, 1)
        _CameraPosition("CameraPosition", Vector) = (0.0, 0.0, 0.0, 0.0)
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 100

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag


            #include "UnityCG.cginc"


            float4 FrontFaceColor;
            float4 BackFaceColor;
            float4 _CameraPosition;

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
            };



            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = UnityObjectToClipPos(v.vertex);
                o.worldpos = mul(unity_ObjectToWorld, v.vertex).xyz;
                o.uv = v.uv;
                o.normal = UnityObjectToWorldNormal(v.normal);

                return o;
            }

            fixed4 frag(v2f i) : SV_Target
            {
                float3 view = normalize(_CameraPosition.xyz - i.worldpos);
                float f = dot(normalize(i.normal), view) > 0.0f ? 1.0f : 0.0f;

                fixed3 col = lerp(BackFaceColor.rgb, FrontFaceColor.rgb, f);

                return fixed4(col, 1.0);
            }
            ENDCG
        }
    }
}
