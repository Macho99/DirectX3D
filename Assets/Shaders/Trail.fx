#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

MeshOutput VS_Trail(VertexMesh input)
{
    MeshOutput output;

    float4 worldPos = input.position;
    output.position = worldPos; // W
    output.worldPosition = output.position;
    output.position = mul(output.position, VP);
    output.uv = input.uv;
	
    output.normal = mul(input.normal, (float3x3) input.world);
    output.normalV = mul(output.normal, (float3x3) V);
    output.positionV = mul(worldPos, V);
    output.viewZ = output.positionV.z;
    output.ssaoPosH = mul(worldPos, VPT);
	
    output.tangent = mul(input.tangent, (float3x3) W);
	
    return output;
}

float4 PS(MeshOutput input) : SV_TARGET
{
	float4 color = DiffuseMap.Sample(LinearSampler, input.uv);
    return color;
}

technique11 Draw
{
	PASS_RS_VP(P0, NoCull, VS_Trail, PS)
};

technique11 Shadow
{
	PASS_SHADOW_V(P0, VS_Trail)
};

technique11 NormalDepth
{
	PASS_RS_VP(P0, NoCull, VS_Trail, PS)
};