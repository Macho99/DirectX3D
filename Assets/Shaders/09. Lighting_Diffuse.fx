#include "00. Global.fx"

float3 LightDir;
float4 LightDiffuse;
float4 MaterialDiffuse;
Texture2D DiffuseMap;

//float4 LightAmbient;
//float4 MaterialAmbient;

VertexOutput VS(VertexTextureNormal input)
{
	VertexOutput output;
	output.position = mul(input.position, W);
	output.position = mul(output.position, VP);

	output.uv = input.uv;
	output.normal = mul(input.normal, (float3x3)W);

	return output;
}

float4 PS(VertexOutput input) : SV_TARGET
{
	float4 color = DiffuseMap.Sample(LinearSampler, input.uv);
	float dotValue = dot(-LightDir, input.normal);
	color = dotValue * color * LightDiffuse * MaterialDiffuse;

	return color;
}

technique11 T0
{
	PASS_VP(P0, VS, PS)

	pass P1
	{
		SetRasterizerState(FillModeWireFrame);
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
};