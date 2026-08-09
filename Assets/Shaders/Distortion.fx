#include "00. Global.fx"
#include "00. Light.fx"

SamplerState LinearClampSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

VertexOut VS(VertexTextureNormalTangent input)
{
    VertexOut output;
    output.position = float4(input.position.xy * 2.0f, 0.0f, 1.0f);
    output.uv = input.uv;
    return output;
}

float4 PS(VertexOut input) : SV_Target
{
    float2 offset = SpecularMap.SampleLevel(PointSampler, input.uv, 0).rg;
    float2 distortedUv = saturate(input.uv + offset);
    return DiffuseMap.Sample(LinearClampSampler, distortedUv);
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
