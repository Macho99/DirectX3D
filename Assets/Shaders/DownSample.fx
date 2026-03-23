#include "00. Global.fx"
#include "00. Light.fx"

cbuffer BlurBuffer
{
    float gTexelWidth;
    float gTexelHeight;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float2 Tex : TEXCOORD;
};

VertexOut VS(VertexTextureNormalTangent vin)
{
	VertexOut vout;
    matrix scaleMatrix =
    {
        2, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 2, 0,
        0, 0, 0, 1
    };
    vout.PosH = mul(float4(vin.position.xyz, 1.0f), scaleMatrix);
	vout.Tex = vin.uv;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // 4 sample positions (centered box)
    float2 off = float2(gTexelWidth, gTexelHeight) * 0.5f; // ¹Ý-texel offset
    float2 uv0 = pin.Tex + float2(-off.x, -off.y);
    float2 uv1 = pin.Tex + float2(off.x, -off.y);
    float2 uv2 = pin.Tex + float2(-off.x, off.y);
    float2 uv3 = pin.Tex + float2(off.x, off.y);

    float3 s0 = DiffuseMap.Sample(BorderBlackSampler, uv0).rgb;
    float3 s1 = DiffuseMap.Sample(BorderBlackSampler, uv1).rgb;
    float3 s2 = DiffuseMap.Sample(BorderBlackSampler, uv2).rgb;
    float3 s3 = DiffuseMap.Sample(BorderBlackSampler, uv3).rgb;

    float3 avg = (s0 + s1 + s2 + s3) * 0.25;

    //float lum = Luminance(avg);
    //float mask = SoftThreshold(lum); // 0..1

    //float3 outColor = avg * mask;

    return float4(avg, 1.0);
}

technique11 T0
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}