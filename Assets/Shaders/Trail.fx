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

float4 PS(TrailOutput input) : SV_TARGET
{
	float4 color = DiffuseMap.Sample(LinearSampler, input.uv);
    color = color * Material.diffuse;
    float age = max(Time - input.spawnTime, 0.f);
    float opacity = saturate(1.f - age / max(input.lifetime, 0.001f));
    color.a *= opacity;
    
    if (color.a < 0.01f)
        discard;

    return color;
}

technique11 Draw
{
	pass P0
	{
		SetRasterizerState(NoCull);
		SetBlendState(AlphaBlend, float4(0, 0, 0, 0), 0xFF);
		//SetDepthStencilState(NoDepthWrites, 0);
		SetVertexShader(CompileShader(vs_5_0, VS_Trail()));
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
};

technique11 Shadow
{
	PASS_SHADOW_V(P0, VS_Trail)
};

technique11 NormalDepth
{
	PASS_RS_BS_VP(P0, NoCull, AlphaBlend, VS_Trail, PS)
};
