#include "00. Global.fx"
#include "00. Light.fx"

SamplerState samLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
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

float GetBloomCurve(float intensity)
{
    float result = intensity;
    intensity *= 2.0f;
    
    result = intensity * 0.05 + max(0, intensity - 3) * 0.5;
    
    return result * 0.5f;
}

float4 PS(VertexOut pin) : SV_Target
{
	float3 color = DiffuseMap.Sample(samLinear, pin.Tex).rgb;
	
    float intensity = dot(color, float3(0.3f, 0.3f, 0.3f));
    float bloomIntensity = GetBloomCurve(intensity);
    float3 bloomColor = color * bloomIntensity / max(intensity, 0.0001f);
    
    return float4(bloomColor, 1.0f);
}

technique11 ViewArgbTech
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}