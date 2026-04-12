#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

float4 PS(MeshOutput input) : SV_TARGET
{
    float4 litcolor = float4(input.tangent, 1.0f);
	
    float shadow = CalcCascadeShadowFactor(input.worldPosition, input.viewZ);
    //float4 color = ComputeLight(input.normal, litcolor, input.worldPosition, input.ssaoPosH, shadow);
    return litcolor;
}

float4 NormalDepthPS(MeshOutput input) : SV_TARGET
{
    input.normalV = normalize(input.normalV);
    return float4(input.normalV, input.positionV.z);
}

technique11 Draw
{
	PASS_VP(P0, VS_Mesh, PS)
};

technique11 Shadow
{
	PASS_SHADOW_V(P0, VS_Mesh)
};

technique11 NormalDepth
{
	PASS_VP(P0, VS_Mesh, NormalDepthPS)
};