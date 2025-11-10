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
    matrix VPT;
	matrix VInv;
	float3 CamPos; // 그림자 그릴때 라이트가 아닌 카메라 위치가 필요(빌보드)
	float padding;
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
	float4 shadowPosH : TEXCOORD1;
    float4 ssaoPosH : TEXCOORD2;
    float3 normalV : NORMAL_V;
    float3 positionV : POSITION_V;
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

SamplerState BorderBlackSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Border;
    AddressV = Border;
    BorderColor = float4(0, 0, 0, 0);
};

////////////////
/// RasterizerState
////////////////

RasterizerState FillModeWireFrame
{
	FillMode = WireFrame;
};

RasterizerState NoCull
{
    CullMode = NONE; // 앞, 뒤 컬링 안 함
};

RasterizerState FrontCounterClockwiseTrue
{
	FrontCounterClockwise = true;
};

RasterizerState Depth
{
	// [From MSDN]
	// If the depth buffer currently bound to the output-merger stage has a UNORM format or
	// no depth buffer is bound the bias value is calculated like this: 
	//
	// Bias = (float)DepthBias * r + SlopeScaledDepthBias * MaxDepthSlope;
	//
	// where r is the minimum representable value > 0 in the depth-buffer format converted to float32.
	// [/End MSDN]
	// 
	// For a 24-bit depth buffer, r = 1 / 2^24.
	//
	// Example: DepthBias = 100000 ==> Actual DepthBias = 100000/2^24 = .006

	// You need to experiment with these values for your scene.
	DepthBias = 1000;
	DepthBiasClamp = 0.0f;
	SlopeScaledDepthBias = 1.0f;
};

RasterizerState DepthNoCull
{
	DepthBias = 1000;
	DepthBiasClamp = 0.0f;
	SlopeScaledDepthBias = 1.0f;
	CullMode = NONE; // 앞, 뒤 컬링 안 함
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

#define PASS_SHADOW_V(name, vs)						\
pass name											\
{													\
	SetVertexShader(CompileShader(vs_5_0, vs()));	\
	SetPixelShader(NULL);							\
	SetRasterizerState(Depth);						\
}

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
	return CamPos;
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