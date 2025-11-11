#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

float4 AlphaClipPS(MeshOutput input) : SV_TARGET
{
    float shadow = CalcShadowFactor(ShadowMap, input.shadowPosH);
    float4 color = ComputeLight(input.normal, input.uv, input.worldPosition, input.ssaoPosH, shadow);
	
	if(color.a < 0.1f)
        discard;
	
    return color;
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
	PASS_RS_VP(P0, NoCull, VS_Mesh, AlphaClipPS)
};

technique11 Shadow
{
	PASS_RS_VP(P0, Depth, VS_Mesh, AlphaClipPS)
};

technique11 NormalDepth
{
	PASS_RS_VP(P0, NoCull, VS_Mesh, AlphaClipNormalDepthPS)
};