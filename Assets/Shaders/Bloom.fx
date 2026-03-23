#include "00. Global.fx"
#include "00. Light.fx"

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
    float3 color = DiffuseMap.Sample(BorderBlackSampler, pin.Tex).rgb;
    float threshold = 1.0f;
    float softKnee = 0.5f;
    
    // Luminance
    float luma = dot(color, float3(0.299f, 0.587f, 0.114f));
    
    // HDR ∞Óº±: π‡¿∫ ∫Œ∫–∏∏ √ﬂ√‚
    float response = max(0.0f, luma - threshold) / (luma + 1.0f);
    
    return float4(color * response, 1.0f);
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