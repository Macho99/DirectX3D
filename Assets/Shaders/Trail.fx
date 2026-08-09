#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

struct TrailVertex
{
    float4 position : POSITION;
    float2 uv : TEXCOORD0;
    float spawnTime : TEXCOORD1;
    float lifetime : TEXCOORD2;
};

struct TrailOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float spawnTime : TEXCOORD1;
    float lifetime : TEXCOORD2;
};

TrailOutput VS_Trail(TrailVertex input)
{
    TrailOutput output;

    output.position = mul(input.position, VP);
    output.uv = input.uv;
    output.spawnTime = input.spawnTime;
    output.lifetime = input.lifetime;

    return output;
}

float4 PS_Distortion(TrailOutput input) : SV_TARGET
{
    float mask = DiffuseMap.Sample(LinearSampler, input.uv).a;
    float age = max(Time - input.spawnTime, 0.f);
    float opacity = saturate(1.f - age / max(input.lifetime, 0.001f));
    mask *= opacity;

    if (mask < 0.01f)
        discard;

    float2 trailTangent = float2(ddx(input.uv.x), ddy(input.uv.x));
    float tangentLength = length(trailTangent);
    if (tangentLength < 0.00001f)
        discard;

    float2 trailNormal = float2(-trailTangent.y, trailTangent.x) / tangentLength;
    float signedDistanceFromCenter = input.uv.y * 2.f - 1.f;
    float strength = Material.diffuse.x * 0.001f;
    float2 offset = trailNormal * signedDistanceFromCenter * strength;
	float2 encodedOffset = saturate(offset / (MaxDistortionOffset * 2.f) + 0.5f);

    return float4(encodedOffset, 0.f, mask);
}

technique11 Distortion
{
	pass P0
	{
		SetRasterizerState(NoCull);
		SetBlendState(AlphaBlend, float4(0, 0, 0, 0), 0xFF);
		SetDepthStencilState(NoDepthWrites, 0);
		SetVertexShader(CompileShader(vs_5_0, VS_Trail()));
		SetPixelShader(CompileShader(ps_5_0, PS_Distortion()));
	}
};
