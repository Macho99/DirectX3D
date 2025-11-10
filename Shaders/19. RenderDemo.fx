#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

float4 PS(MeshOutput input) : SV_TARGET
{
	float shadow = CalcShadowFactor(ShadowMap, input.shadowPosH);
	float4 color = ComputeLight(input.normal, input.uv, input.worldPosition, input.ssaoPosH, shadow);
	//float4 color = DiffuseMap.Sample(LinearSampler, input.uv);
	
	return color;
}

float4 AlphaClipPS(MeshOutput input) : SV_TARGET
{
    float shadow = CalcShadowFactor(ShadowMap, input.shadowPosH);
    float4 color = ComputeLight(input.normal, input.uv, input.worldPosition, input.ssaoPosH, shadow);
	
	if(color.a < 0.1f)
        discard;
	
    return color;
}

float4 NormalDepthPS(MeshOutput input) : SV_TARGET
{
    input.normalV = normalize(input.normalV);
    return float4(input.normalV, input.positionV.z);
}

float4 AlphaClipNormalDepthPS(MeshOutput input) : SV_TARGET
{
    input.normalV = normalize(input.normalV);
    float4 color = DiffuseMap.Sample(LinearSampler, input.uv);
	
    if (color.a < 0.1f)
        discard;
	
    return float4(input.normalV, input.positionV.z);
}

technique11 Draw
{
	PASS_VP(P0, VS_Mesh, PS)
	PASS_VP(P1, VS_Model, PS)
	PASS_VP(P2, VS_Animation, PS)
	PASS_RS_VP(P3, NoCull, VS_Model, AlphaClipPS)
};

technique11 Shadow
{
	PASS_SHADOW_V(P0, VS_Mesh)
	PASS_SHADOW_V(P1, VS_Model)
	PASS_SHADOW_V(P2, VS_Animation)
	PASS_SHADOW_V(P3, VS_Model)
};

technique11 NormalDepth
{
	PASS_VP(P0, VS_Mesh, NormalDepthPS)
	PASS_VP(P1, VS_Model, NormalDepthPS)
	PASS_VP(P2, VS_Animation, NormalDepthPS)
	PASS_RS_VP(P3, NoCull, VS_Model, AlphaClipNormalDepthPS)
};