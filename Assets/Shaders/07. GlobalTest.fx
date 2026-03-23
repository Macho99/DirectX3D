#include "00. Global.fx"

Texture2D Texture0;
float3 LightDir;

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
	return Texture0.Sample(LinearSampler, input.uv);
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