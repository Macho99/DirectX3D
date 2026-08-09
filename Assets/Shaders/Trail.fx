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

float4 SampleTrail(TrailOutput input)
{
    float4 trail = DiffuseMap.Sample(LinearSampler, input.uv);
    float age = max(Time - input.spawnTime, 0.f);
    float opacity = saturate(1.f - age / max(input.lifetime, 0.001f));
    trail.a *= opacity * Material.diffuse.a;

    if (trail.a < 0.01f)
        discard;

    return trail;
}

void GetTrailFrame(TrailOutput input, out float2 trailTangent, out float2 trailNormal)
{
    trailTangent = float2(ddx(input.uv.x), ddy(input.uv.x));
    float tangentLength = length(trailTangent);
    if (tangentLength < 0.00001f)
        discard;

    trailTangent /= tangentLength;
    trailNormal = float2(-trailTangent.y, trailTangent.x);
}

float4 PS_Draw(TrailOutput input) : SV_TARGET
{
    float4 trail = SampleTrail(input);
    trail.rgb *= Material.diffuse.rgb;
    return trail;
}

float4 PS_Distortion(TrailOutput input) : SV_TARGET
{
    float mask = SampleTrail(input).a;

    float2 trailTangent;
    float2 trailNormal;
    GetTrailFrame(input, trailTangent, trailNormal);

    float2 noiseUv = float2(input.uv.x * 2.f - Time * 1.5f, input.uv.y);
    float2 noise = NormalMap.Sample(LinearSampler, noiseUv).rg * 2.f - 1.f;
    float2 noiseDirection = trailTangent * noise.x + trailNormal * noise.y;

    float signedDistanceFromCenter = input.uv.y * 2.f - 1.f;
    float2 baseDirection = trailNormal * signedDistanceFromCenter;
    float2 distortionDirection = lerp(baseDirection, noiseDirection, 0.35f);
    float strength = Material.specular.x;
	float2 offset = distortionDirection * strength;
	float2 encodedOffset = saturate(offset / (MaxDistortionOffset * 2.f) + 0.5f);

    return float4(encodedOffset, 0.f, mask);
}

technique11 Draw
{
	pass P0
	{
		SetRasterizerState(NoCull);
		SetBlendState(AlphaBlend, float4(0, 0, 0, 0), 0xFF);
		SetDepthStencilState(NoDepthWrites, 0);
		SetVertexShader(CompileShader(vs_5_0, VS_Trail()));
		SetPixelShader(CompileShader(ps_5_0, PS_Draw()));
	}
};

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
