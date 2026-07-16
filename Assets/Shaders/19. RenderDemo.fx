#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

float4 PS(MeshOutput input) : SV_TARGET
{
    float3 worldNormal = ComputeNormalMapping(input.normal, input.tangent, input.uv);
	float shadow = CalcCascadeShadowFactor(input.worldPosition, input.viewZ);
    float4 color = ComputeLitAndLight(worldNormal, input.uv, input.worldPosition, input.ssaoPosH, shadow);
	//float4 color = DiffuseMap.Sample(LinearSampler, input.uv);
	
	return color;
}

float4 NormalDepthPS(MeshOutput input) : SV_TARGET
{
    float3 worldNormal = ComputeNormalMapping(input.normal, input.tangent, input.uv);
    float3 viewNormal = mul(worldNormal, (float3x3) V);
    return float4(viewNormal, input.position.z);
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

technique11 NormalDepth
{
	PASS_VP(P0, VS_Mesh, NormalDepthPS)
	PASS_VP(P1, VS_Model, NormalDepthPS)
	PASS_VP(P2, VS_Animation, NormalDepthPS)
};