#include "00. Global.fx"


float4 MaterialEmissive;

MeshOutput VS(VertexTextureNormal input)
{
	MeshOutput output;
	output.position = mul(input.position, W);
	output.worldPosition = output.position;
	output.position = mul(output.position, VP);

	output.uv = input.uv;
	output.normal = mul(input.normal, (float3x3)W);

	return output;
}

float4 PS(MeshOutput input) : SV_TARGET
{ 
	float3 camPosition = -V._41_42_43;
	float3 E = normalize(camPosition - input.worldPosition);

	float value = saturate(dot(E, input.normal));
	float emissive = 1.0f - value;

	emissive = smoothstep(0.0f, 1.0f, emissive);

	float color = MaterialEmissive * emissive;

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