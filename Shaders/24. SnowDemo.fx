#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

cbuffer SnowBuffer
{
	float4 Color;
	float3 Velocity;
	float DrawDistance;

	float3 Origin;
	float Turbulence;

	float3 Extent;
	float Time;
};

struct VertexInput
{
	float4 position : POSITION;
	float2 uv : UV;
	float2 scale : SCALE;
	float2 random : RANDOM;
};

struct V_OUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
	float alpha : ALPHA;
};

float3 FindPointInWindow(float3 center, float3 extent, float3 position)
{
	float3 ijk = round((center - position) / extent);
	float3 result = extent * ijk + position;
	return result;
}

V_OUT VS(VertexInput input)
{
	V_OUT output;
	
	input.position.xyz += Velocity * Time;
	//input.position.y = Origin.y + Extent.y - (input.position.y - displace) % Extent.y;
	input.position.x += cos(Time - input.random.x) * Turbulence;
	input.position.z += cos(Time - input.random.y) * Turbulence;
	//input.position.xyz = Origin + (Extent + (input.position.xyz + displace) % Extent) % Extent - (Extent * 0.5f);
	input.position.xyz = FindPointInWindow(Origin, Extent, input.position.xyz);

	float4 position = mul(input.position, W);
	
	float3 up = float3(0, 1, 0);
	//float3 forward = float3(0, 0, 1);
	float3 forward = position.xyz - CameraPosition(); // BillBoard
	float3 right = normalize(cross(up, forward));

	position.xyz += (input.uv.x - 0.5f) * right * input.scale.x;
	position.xyz += (1.0f - input.uv.y - 0.5f) * up * input.scale.y;
	position.w = 1.0f;

	output.position = mul(mul(position, V), P);
	output.uv = input.uv;

	output.alpha = 1.0f;

	// Alpha Blending
	float4 view = mul(position, V);
	output.alpha = saturate(1 - view.z / DrawDistance) * 0.8f;

	return output;
}

float4 PS(V_OUT input) : SV_Target
{
	float4 diffuse = DiffuseMap.Sample(LinearSampler, input.uv);

	diffuse.rgb = Color.rgb * input.alpha * 2.0f;
	diffuse.a = diffuse.a * input.alpha;
	
	if(diffuse.a < 0.1f)
		discard;

	return diffuse;
}

technique11 Draw
{
	PASS_BS_VP(P0, AlphaBlend, VS, PS)
};