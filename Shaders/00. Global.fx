#ifndef _GLOBAL_FX_
#define _GLOBAL_FX_


Texture1D RandomMap;

////////////////
/// ConstBuffer
////////////////

cbuffer GlobalBuffer
{
	matrix V;
	matrix P;
	matrix VP;
	matrix VInv;
};

cbuffer TransformBuffer
{
	matrix W;
};

////////////////
/// VertexData
////////////////

struct Vertex
{
	float4 position : POSITION;
};

struct VertexTexture
{
	float4 position : POSITION;
	float2 uv : TEXCOORD;
};

struct VertexColor
{
	float4 position : POSITION;
	float4 color : COLOR;
};

struct VertexTextureNormal
{
	float4 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
};

struct VertexTextureNormalTangent
{
	float4 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

struct VertexTextureNormalTangentBlend
{
	float4 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float4 blendIndices : BLEND_INDICES;
	float4 blendWeights : BLEND_WEIGHTS;
};

////////////////
/// VertexOutput
////////////////
	
struct VertexOutput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
};

struct MeshOutput
{
	float4 position : SV_POSITION;
	float3 worldPosition : POSITION1;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

////////////////
/// SamplerState
////////////////

SamplerState LinearSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

SamplerState PointSampler
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Wrap;
	AddressV = Wrap;
};

////////////////
/// RasterizerState
////////////////

RasterizerState FillModeWireFrame
{
	FillMode = WireFrame;
};

RasterizerState FrontCounterClockwiseTrue
{
	FrontCounterClockwise = true;
};

////////////////
// BlendState //
////////////////

BlendState AlphaBlend
{
	AlphaToCoverageEnable = false;

	BlendEnable[0] = true;
	SrcBlend[0] = SRC_ALPHA;
	DestBlend[0] = INV_SRC_ALPHA;
	BlendOp[0] = ADD;

	SrcBlendAlpha[0] = One;
	DestBlendAlpha[0] = Zero;
	BlendOpAlpha[0] = Add;

	RenderTargetWriteMask[0] = 15;
};

BlendState AlphaBlendAlphaToCoverageEnable
{
	AlphaToCoverageEnable = true;

	BlendEnable[0] = true;
	SrcBlend[0] = SRC_ALPHA;
	DestBlend[0] = INV_SRC_ALPHA;
	BlendOp[0] = ADD;

	SrcBlendAlpha[0] = One;
	DestBlendAlpha[0] = Zero;
	BlendOpAlpha[0] = Add;

	RenderTargetWriteMask[0] = 15;
};

BlendState AdditiveBlend
{
/*
	AlphaToCoverageEnable = true;

	BlendEnable[0] = true;
	SrcBlend[0] = One;
	DestBlend[0] = One;
	BlendOp[0] = ADD;

	SrcBlendAlpha[0] = One;
	DestBlendAlpha[0] = Zero;
	BlendOpAlpha[0] = Add;

	RenderTargetWriteMask[0] = 15;
*/
	AlphaToCoverageEnable = FALSE;
	BlendEnable[0] = TRUE;
	SrcBlend = SRC_ALPHA;
	DestBlend = ONE;
	BlendOp = ADD;
	SrcBlendAlpha = ZERO;
	DestBlendAlpha = ZERO;
	BlendOpAlpha = ADD;
	RenderTargetWriteMask[0] = 0x0F;

};

BlendState AdditiveBlendAlphaToCoverageEnable
{
	AlphaToCoverageEnable = true;

	BlendEnable[0] = true;
	SrcBlend[0] = One;
	DestBlend[0] = One;
	BlendOp[0] = ADD;

	SrcBlendAlpha[0] = One;
	DestBlendAlpha[0] = Zero;
	BlendOpAlpha[0] = Add;

	RenderTargetWriteMask[0] = 15;
};

///////////////////////
// DepthStencilState //
///////////////////////

DepthStencilState DisableDepth
{
	DepthEnable = FALSE;
	DepthWriteMask = ZERO;
};

DepthStencilState NoDepthWrites
{
	DepthEnable = TRUE;
	DepthWriteMask = ZERO;
};

////////////////
/// Macro
////////////////

#define PASS_VP(name, vs, ps)						\
pass name											\
{													\
	SetVertexShader(CompileShader(vs_5_0, vs()));	\
	SetPixelShader(CompileShader(ps_5_0, ps()));	\
}

#define PASS_RS_VP(name, rs, vs, ps)				\
pass name											\
{													\
    SetRasterizerState(rs);							\
    SetVertexShader(CompileShader(vs_5_0, vs()));	\
    SetPixelShader(CompileShader(ps_5_0, ps()));	\
}

#define PASS_BS_VP(name, bs, vs, ps)				\
pass name											\
{													\
	SetBlendState(bs, float4(0, 0, 0, 0), 0xFF);	\
    SetVertexShader(CompileShader(vs_5_0, vs()));	\
    SetPixelShader(CompileShader(ps_5_0, ps()));	\
}

////////////////
/// Function
////////////////

float3 CameraPosition()
{
	return VInv._41_42_43;
}

float3 RandUnitVec3(float gameTime, float offset)
{
	// Use game time plus offset to sample random texture.
	float u = (gameTime + offset);

	// coordinates in [-1,1]
	float3 v = RandomMap.SampleLevel(LinearSampler, u, 0).xyz;

	// project onto unit sphere
	return normalize(v);
}
#endif