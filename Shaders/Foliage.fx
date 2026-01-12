#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"
#include "00. GrassCommon.fx"

MeshOutput VS(VertexMesh input)
{
    MeshOutput output;
    
    float4 worldPos = mul(input.position, input.world);
    
    float3 windOffset = CalculateWindOffset(worldPos.xyz, input.position.y);
    worldPos.xyz += windOffset;
    output.position = worldPos; // W
    output.worldPosition = output.position;
    output.position = mul(output.position, VP);
    output.uv = input.uv;
	
    output.normal = mul(input.normal, (float3x3) W);
    output.normalV = mul(output.normal, (float3x3) V);
    output.positionV = mul(worldPos, V);
    output.viewZ = output.positionV.z;
    output.ssaoPosH = mul(worldPos, VPT);
	
    return output;
}

void AlphaClipShadowPS(MeshOutput input)
{
    float4 litColor = DiffuseMap.Sample(LinearSampler, input.uv);
	
    if (litColor.a < 0.1f)
        discard;
}

float4 AlphaClipPS(MeshOutput input,
    bool isFrontFace : SV_IsFrontFace) : SV_TARGET
{
    float4 litColor = DiffuseMap.Sample(LinearSampler, input.uv);
    if (litColor.a < 0.1f)
        discard;
    
    float shadow = CalcCascadeShadowFactor(input.worldPosition, input.viewZ);
    if (isFrontFace == false)
        input.normal = -input.normal;
    float4 color = ComputeLight(input.normal, litColor, input.worldPosition, input.ssaoPosH, shadow);
	
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
	PASS_RS_VP(P0, NoCull, VS, AlphaClipPS)
};

technique11 Shadow
{
	PASS_RS_VP(P0, Depth, VS, AlphaClipShadowPS)
};

technique11 NormalDepth
{
	PASS_RS_VP(P0, NoCull, VS, AlphaClipNormalDepthPS)
};