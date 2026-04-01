#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

struct VertexOut
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

VertexOut VS_Line(VertexColor input)
{
    VertexOut output;
    output.pos = mul(input.position, VP);
    output.color = input.color;
    return output;
}

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

float4 PS(PS_IN input) : SV_TARGET
{
    return input.color;
}

technique11 Draw
{
	PASS_VP(P0, VS_Line, PS)
};

technique11 Shadow
{
	PASS_SHADOW_V(P0, VS_Line)
};

technique11 NormalDepth
{
	PASS_VP(P0, VS_Line, PS)
};