#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

float4 PS(MeshOutput input) : SV_TARGET
{
	float shadow = CalcShadowFactor(ShadowMap, input.shadowPosH);
	float4 color = ComputeLight(input.normal, input.uv, input.worldPosition, shadow);
	//float4 color = DiffuseMap.Sample(LinearSampler, input.uv);
	
	return color;
}

technique11 Draw
{
	PASS_VP(P0, VS_Mesh, PS)
	PASS_VP(P1, VS_Model, PS)
	PASS_VP(P2, VS_Animation, PS)
};

technique11 Shadow
{
	PASS_SHADOW_V(P0, VS_Mesh)
	PASS_SHADOW_V(P1, VS_Model)
	PASS_SHADOW_V(P2, VS_Animation)
};
