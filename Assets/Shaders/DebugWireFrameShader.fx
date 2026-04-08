#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

float4 PS(MeshOutput input) : SV_TARGET
{
    float4 color = float4(input.tangent, 1.0f);
	
	return color;
}

float4 NormalDepthPS(MeshOutput input) : SV_TARGET
{
    input.normalV = normalize(input.normalV);
    return float4(input.normalV, input.positionV.z);
}

technique11 Draw
{
	PASS_RS_VP(P0, NoCullWireFrame, VS_Mesh, PS)
};

technique11 Shadow
{
	PASS_SHADOW_V(P0, VS_Mesh)
};

technique11 NormalDepth
{
	PASS_VP(P0, VS_Mesh, NormalDepthPS)
};