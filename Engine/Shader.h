#pragma once
#include "Pass.h"
#include "Technique.h"
#include "BindShaderDesc.h"

struct ShaderDesc
{
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DX11Effect> effect;
};

enum class RenderTech
{
	Shadow = 0,
	Draw,
	NormalDepth,
	Max
};

class Shader
{
public:
	friend struct Pass;

public:
	Shader(wstring file);
	~Shader();

	wstring GetFile() { return _file; }
	ComPtr<ID3DX11Effect> Effect() { return _shaderDesc.effect; }

	void SetTechNum(RenderTech renderTech, int techNum);
	bool CanDraw(RenderTech renderTech);
	void Draw(UINT technique, UINT pass, UINT vertexCount, UINT startVertexLocation = 0);
	void DrawIndexed(UINT technique, UINT pass, UINT indexCount, UINT startIndexLocation = 0, INT baseVertexLocation = 0);
	void DrawIndexed(RenderTech renderTech, UINT pass, UINT indexCount, UINT startIndexLocation = 0, INT baseVertexLocation = 0);
	void DrawInstanced(UINT technique, UINT pass, UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation = 0, UINT startInstanceLocation = 0);
	void DrawIndexedInstanced(UINT technique, UINT pass, UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation = 0, INT baseVertexLocation = 0, UINT startInstanceLocation = 0);
	void DrawIndexedInstanced(RenderTech renderTech, UINT pass, UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation = 0, INT baseVertexLocation = 0, UINT startInstanceLocation = 0);
	void BeginDraw(UINT technique, UINT pass);
	void EndDraw(UINT technique, UINT pass);

	void Dispatch(UINT technique, UINT pass, UINT x, UINT y, UINT z);

	ComPtr<ID3DX11EffectVariable> GetVariable(string name);
	ComPtr<ID3DX11EffectScalarVariable> GetScalar(string name);
	ComPtr<ID3DX11EffectVectorVariable> GetVector(string name);
	ComPtr<ID3DX11EffectMatrixVariable> GetMatrix(string name);
	ComPtr<ID3DX11EffectStringVariable> GetString(string name);
	ComPtr<ID3DX11EffectShaderResourceVariable> GetSRV(string name);
	ComPtr<ID3DX11EffectRenderTargetViewVariable> GetRTV(string name);
	ComPtr<ID3DX11EffectDepthStencilViewVariable> GetDSV(string name);
	ComPtr<ID3DX11EffectUnorderedAccessViewVariable> GetUAV(string name);
	ComPtr<ID3DX11EffectConstantBuffer> GetConstantBuffer(string name);
	ComPtr<ID3DX11EffectShaderVariable> GetShader(string name);
	ComPtr<ID3DX11EffectBlendVariable> GetBlend(string name);
	ComPtr<ID3DX11EffectDepthStencilVariable> GetDepthStencil(string name);
	ComPtr<ID3DX11EffectRasterizerVariable> GetRasterizer(string name);
	ComPtr<ID3DX11EffectSamplerVariable> GetSampler(string name);

private:
	void CreateEffect();
	ComPtr<ID3D11InputLayout> CreateInputLayout(ComPtr<ID3DBlob> fxBlob, D3DX11_EFFECT_SHADER_DESC* effectVsDesc, vector<D3D11_SIGNATURE_PARAMETER_DESC>& params);

private:
	wstring _file;
	ShaderDesc _shaderDesc;
	D3DX11_EFFECT_DESC _effectDesc;
	shared_ptr<StateBlock> _initialStateBlock;
	vector<Technique> _techniques;

public:
	void PushGlobalData(const Matrix& view, const Matrix& projection);
	void PushTransformData(const TransformDesc& desc);
	void PushLightData(const LightDesc& desc);
	void PushMaterialData(const MaterialDesc& desc);
	void PushBoneData(const BoneDesc& desc);
	void PushKeyframeData(const KeyframeDesc& desc);
	void PushTweenData(const InstancedTweenDesc& desc);
	void PushSnowData(const SnowBillboardDesc& desc);
	void PushParticleData(const ParticleDesc& desc);
	void PushShadowData(const Matrix& desc);
	void PushSsaoData(const SsaoDesc& desc);
	void PushBlurData(const BlurDesc& desc);

	vector<Technique>& GetTechniques() { return _techniques; }

	int GetTechNum(RenderTech renderTech);

private:
	GlobalDesc _globalDesc;
	shared_ptr<ConstantBuffer<GlobalDesc>> _globalBuffer;
	ComPtr<ID3DX11EffectConstantBuffer> _globalEffectBuffer;

	TransformDesc _transformDesc;
	shared_ptr<ConstantBuffer<TransformDesc>> _transformBuffer;
	ComPtr<ID3DX11EffectConstantBuffer> _transformEffectBuffer;

	LightDesc _lightDesc;
	shared_ptr<ConstantBuffer<LightDesc>> _lightBuffer;
	ComPtr<ID3DX11EffectConstantBuffer> _lightEffectBuffer;

	MaterialDesc _materialDesc;
	shared_ptr<ConstantBuffer<MaterialDesc>> _materialBuffer;
	ComPtr<ID3DX11EffectConstantBuffer> _materialEffectBuffer;

	BoneDesc _boneDesc;
	shared_ptr<ConstantBuffer<BoneDesc>> _boneBuffer;
	ComPtr<ID3DX11EffectConstantBuffer> _boneEffectBuffer;

	KeyframeDesc _keyframeDesc;
	shared_ptr<ConstantBuffer<KeyframeDesc>> _keyframeBuffer;
	ComPtr<ID3DX11EffectConstantBuffer> _keyframeEffectBuffer;

	InstancedTweenDesc _tweenDesc;
	shared_ptr<ConstantBuffer<InstancedTweenDesc>> _tweenBuffer;
	ComPtr<ID3DX11EffectConstantBuffer> _tweenEffectBuffer;

	SnowBillboardDesc _snowDesc;
	shared_ptr<ConstantBuffer<SnowBillboardDesc>> _snowBuffer;
	ComPtr<ID3DX11EffectConstantBuffer> _snowEffectBuffer;

	ParticleDesc _particleDesc;
	shared_ptr<ConstantBuffer<ParticleDesc>> _particleBuffer;
	ComPtr<ID3DX11EffectConstantBuffer> _particleEffectBuffer;

	Matrix _shadowDesc;
	shared_ptr<ConstantBuffer<Matrix>> _shadowBuffer;
	ComPtr<ID3DX11EffectConstantBuffer> _shadowEffectBuffer;

	SsaoDesc _ssaoDesc;
	shared_ptr<ConstantBuffer<SsaoDesc>> _ssaoBuffer;
	ComPtr<ID3DX11EffectConstantBuffer> _ssaoEffectBuffer;

	BlurDesc _blurDesc;
	shared_ptr<ConstantBuffer<BlurDesc>> _blurBuffer;
	ComPtr<ID3DX11EffectConstantBuffer> _blurEffectBuffer;

	int _techNums[static_cast<int>(RenderTech::Max)];
};

class ShaderManager
{
public:
	static ShaderDesc GetEffect(wstring fileName);

private:
	static unordered_map<wstring, ShaderDesc> shaders;
};