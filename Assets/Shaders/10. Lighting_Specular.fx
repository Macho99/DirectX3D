#include "00. Global.fx"

float3 LightDir;
float4 LightSpecular;
float4 MaterialSpecular;

Texture2D DiffuseMap;

//float4 LightAmbient;
//float4 MaterialAmbient;

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
	//float3 R = reflect(LightDir, input.normal);
	float3 R = LightDir - (2 * dot(LightDir, input.normal) * input.normal);
	R = normalize(R);

	float3 camPosition = -V._41_42_43;
	float3 E = normalize(camPosition - input.worldPosition);

	float value = saturate(dot(R, E)); // clamp01
	float specular = pow(value, 10);


	float4 color = LightSpecular * MaterialSpecular * specular;
	//float4 color = DiffuseMap.Sample(LinearSampler, input.uv);

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