#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

float4 PS(MeshOutput input) : SV_TARGET
{
	//float shadow = CalcCascadeShadowFactor(input.worldPosition, input.viewZ);
	float4 color = ComputeLitAndLight(input.normal, input.uv, input.worldPosition, input.ssaoPosH, 1);
    return color;
}

//float4 NormalDepthPS(MeshOutput input) : SV_TARGET
//{
//    input.normalV = normalize(input.normalV);
//    return float4(input.normalV, input.position.z);
//}

technique11 Draw
{
	PASS_BS_VP(P0, AlphaBlendKeepAlpha, VS_Mesh_NotInst, PS)
};

//technique11 Shadow
//{
//	PASS_SHADOW_V(P0, VS_Mesh)
//	PASS_SHADOW_V(P1, VS_Model)
//	PASS_SHADOW_V(P2, VS_Animation)
//};
//
//technique11 NormalDepth
//{
//	PASS_VP(P0, VS_Mesh, NormalDepthPS)
//	PASS_VP(P1, VS_Model, NormalDepthPS)
//	PASS_VP(P2, VS_Animation, NormalDepthPS)
//};
