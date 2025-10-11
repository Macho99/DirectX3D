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

float4 PS(VertexOut pin) : SV_Target
{
	float4 color = DiffuseMap.Sample(samLinear, pin.Tex);
    float4 color2 = SpecularMap.Sample(samLinear, pin.Tex);
    
    return color + color2;
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