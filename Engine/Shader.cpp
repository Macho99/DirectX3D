#include "pch.h"
#include "Shader.h"
#include "Utils.h"
#include "Camera.h"

Shader::Shader(wstring file) : _file(L"..\\Shaders\\" + file)
{
	_initialStateBlock = make_shared<StateBlock>();
	{
		DC->RSGetState(_initialStateBlock->RSRasterizerState.GetAddressOf());
		DC->OMGetBlendState(_initialStateBlock->OMBlendState.GetAddressOf(), _initialStateBlock->OMBlendFactor, &_initialStateBlock->OMSampleMask);
		DC->OMGetDepthStencilState(_initialStateBlock->OMDepthStencilState.GetAddressOf(), &_initialStateBlock->OMStencilRef);
	}

	CreateEffect();

	for (int i = 0; i < static_cast<int>(RenderTech::Max); i++)
	{
		_techNums[i] = -1;
	}

	for (int i = 0; i < _techniques.size(); i++)
	{
		Technique technique = _techniques[i];
		RenderTech renderTech;
		if (technique.name == L"Shadow")
			renderTech = RenderTech::Shadow;
		else if (technique.name == L"Draw")
			renderTech = RenderTech::Draw;
		else if (technique.name == L"NormalDepth")
			renderTech = RenderTech::NormalDepth;
		else
			continue;

		_techNums[static_cast<int>(renderTech)] = i;
	}
}

Shader::~Shader()
{

}

void Shader::CreateEffect()
{
	_shaderDesc = ShaderManager::GetEffect(_file);

	_shaderDesc.effect->GetDesc(&_effectDesc);
	for (UINT t = 0; t < _effectDesc.Techniques; t++)
	{
		Technique technique;
		technique.technique = _shaderDesc.effect->GetTechniqueByIndex(t);
		technique.technique->GetDesc(&technique.desc);
		technique.name = Utils::ToWString(technique.desc.Name);

		for (UINT p = 0; p < technique.desc.Passes; p++)
		{
			Pass pass;
			pass.pass = technique.technique->GetPassByIndex(p);
			pass.pass->GetDesc(&pass.desc);
			pass.name = Utils::ToWString(pass.desc.Name);
			pass.pass->GetVertexShaderDesc(&pass.passVsDesc);
			pass.passVsDesc.pShaderVariable->GetShaderDesc(pass.passVsDesc.ShaderIndex, &pass.effectVsDesc);

			for (UINT s = 0; s < pass.effectVsDesc.NumInputSignatureEntries; s++)
			{
				D3D11_SIGNATURE_PARAMETER_DESC desc;

				HRESULT hr = pass.passVsDesc.pShaderVariable->GetInputSignatureElementDesc(pass.passVsDesc.ShaderIndex, s, &desc);
				CHECK(hr);

				pass.signatureDescs.push_back(desc);
			}

			pass.inputLayout = CreateInputLayout(_shaderDesc.blob, &pass.effectVsDesc, pass.signatureDescs);
			pass.stateBlock = _initialStateBlock;

			technique.passes.push_back(pass);
		}

		_techniques.push_back(technique);
	}

	for (UINT i = 0; i < _effectDesc.ConstantBuffers; i++)
	{
		ID3DX11EffectConstantBuffer* iBuffer;
		iBuffer = _shaderDesc.effect->GetConstantBufferByIndex(i);

		D3DX11_EFFECT_VARIABLE_DESC vDesc;
		iBuffer->GetDesc(&vDesc);
	}

	for (UINT i = 0; i < _effectDesc.GlobalVariables; i++)
	{
		ID3DX11EffectVariable* effectVariable;
		effectVariable = _shaderDesc.effect->GetVariableByIndex(i);

		D3DX11_EFFECT_VARIABLE_DESC vDesc;
		effectVariable->GetDesc(&vDesc);
	}
}

ComPtr<ID3D11InputLayout> Shader::CreateInputLayout(ComPtr<ID3DBlob> fxBlob, D3DX11_EFFECT_SHADER_DESC* effectVsDesc, vector<D3D11_SIGNATURE_PARAMETER_DESC>& params)
{
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;

	for (D3D11_SIGNATURE_PARAMETER_DESC& paramDesc : params)
	{
		D3D11_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		if (paramDesc.Mask == 1)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if (paramDesc.Mask <= 3)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if (paramDesc.Mask <= 7)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if (paramDesc.Mask <= 15)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		string name = paramDesc.SemanticName;
		std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });

		if (name == "POSITION")
		{
			elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}

		if (Utils::StartsWith(name, "INST") == true)
		{
			elementDesc.InputSlot = 1;
			elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			elementDesc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
			elementDesc.InstanceDataStepRate = 1;
		}

		if (Utils::StartsWith(name, "SV_") == false)
			inputLayoutDesc.push_back(elementDesc);
	}

	const void* code = effectVsDesc->pBytecode;
	UINT codeSize = effectVsDesc->BytecodeLength;

	if (inputLayoutDesc.size() > 0)
	{
		ComPtr<ID3D11InputLayout> inputLayout;

		HRESULT hr = DEVICE->CreateInputLayout
		(
			&inputLayoutDesc[0]
			, inputLayoutDesc.size()
			, code
			, codeSize
			, inputLayout.GetAddressOf()
		);

		CHECK(hr);

		return inputLayout;
	}

	return nullptr;
}

void Shader::SetTechNum(RenderTech renderTech, int techNum)
{
	_techNums[static_cast<int>(renderTech)] = techNum;
}

bool Shader::CanDraw(RenderTech renderTech)
{
	int num = _techNums[static_cast<int>(renderTech)];
	return num != -1;
}

void Shader::Draw(UINT technique, UINT pass, UINT vertexCount, UINT startVertexLocation)
{
	_techniques[technique].passes[pass].Draw(vertexCount, startVertexLocation);
}

void Shader::DrawIndexed(UINT technique, UINT pass, UINT indexCount, UINT startIndexLocation, INT baseVertexLocation)
{
	_techniques[technique].passes[pass].DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
}

void Shader::DrawIndexed(RenderTech renderTech, UINT pass, UINT indexCount, UINT startIndexLocation, INT baseVertexLocation)
{
	UINT technique = GetTechNum(renderTech);
	_techniques[technique].passes[pass].DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
}

void Shader::DrawInstanced(UINT technique, UINT pass, UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation)
{
	_techniques[technique].passes[pass].DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void Shader::DrawIndexedInstanced(UINT technique, UINT pass, UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation)
{
	_techniques[technique].passes[pass].DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void Shader::DrawIndexedInstanced(RenderTech renderTech, UINT pass, UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation)
{
	UINT technique = GetTechNum(renderTech);
	_techniques[technique].passes[pass].DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void Shader::BeginDraw(UINT technique, UINT pass)
{
	_techniques[technique].passes[pass].BeginDraw();
}

void Shader::EndDraw(UINT technique, UINT pass)
{
	_techniques[technique].passes[pass].EndDraw();
}

void Shader::Dispatch(UINT technique, UINT pass, UINT x, UINT y, UINT z)
{
	_techniques[technique].passes[pass].Dispatch(x, y, z);
}

ComPtr<ID3DX11EffectVariable> Shader::GetVariable(string name)
{
	return _shaderDesc.effect->GetVariableByName(name.c_str());
}

ComPtr<ID3DX11EffectScalarVariable> Shader::GetScalar(string name)
{
	return _shaderDesc.effect->GetVariableByName(name.c_str())->AsScalar();
}

ComPtr<ID3DX11EffectVectorVariable> Shader::GetVector(string name)
{
	return _shaderDesc.effect->GetVariableByName(name.c_str())->AsVector();
}

ComPtr<ID3DX11EffectMatrixVariable> Shader::GetMatrix(string name)
{
	return _shaderDesc.effect->GetVariableByName(name.c_str())->AsMatrix();
}

ComPtr<ID3DX11EffectStringVariable> Shader::GetString(string name)
{
	return _shaderDesc.effect->GetVariableByName(name.c_str())->AsString();
}

ComPtr<ID3DX11EffectShaderResourceVariable> Shader::GetSRV(string name)
{
	return _shaderDesc.effect->GetVariableByName(name.c_str())->AsShaderResource();
}

ComPtr<ID3DX11EffectRenderTargetViewVariable> Shader::GetRTV(string name)
{
	return _shaderDesc.effect->GetVariableByName(name.c_str())->AsRenderTargetView();
}

ComPtr<ID3DX11EffectDepthStencilViewVariable> Shader::GetDSV(string name)
{
	return _shaderDesc.effect->GetVariableByName(name.c_str())->AsDepthStencilView();
}

ComPtr<ID3DX11EffectConstantBuffer> Shader::GetConstantBuffer(string name)
{
	return _shaderDesc.effect->GetConstantBufferByName(name.c_str());
}

ComPtr<ID3DX11EffectShaderVariable> Shader::GetShader(string name)
{
	return _shaderDesc.effect->GetVariableByName(name.c_str())->AsShader();
}

ComPtr<ID3DX11EffectBlendVariable> Shader::GetBlend(string name)
{
	return _shaderDesc.effect->GetVariableByName(name.c_str())->AsBlend();
}

ComPtr<ID3DX11EffectDepthStencilVariable> Shader::GetDepthStencil(string name)
{
	return _shaderDesc.effect->GetVariableByName(name.c_str())->AsDepthStencil();
}

ComPtr<ID3DX11EffectRasterizerVariable> Shader::GetRasterizer(string name)
{
	return _shaderDesc.effect->GetVariableByName(name.c_str())->AsRasterizer();
}

ComPtr<ID3DX11EffectSamplerVariable> Shader::GetSampler(string name)
{
	return _shaderDesc.effect->GetVariableByName(name.c_str())->AsSampler();
}

ComPtr<ID3DX11EffectUnorderedAccessViewVariable> Shader::GetUAV(string name)
{
	return _shaderDesc.effect->GetVariableByName(name.c_str())->AsUnorderedAccessView();
}

unordered_map<wstring, ShaderDesc> ShaderManager::shaders;

ShaderDesc ShaderManager::GetEffect(wstring fileName)
{
	if (shaders.count(fileName) == 0)
	{
		ComPtr<ID3DBlob> blob;
		ComPtr<ID3DBlob> error;

		WORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
		shaderFlags |= D3D10_SHADER_DEBUG;
		shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif
		//INT flag = D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY | D3D10_SHADER_PACK_MATRIX_ROW_MAJOR;
		//shaderFlags |= D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY;
		shaderFlags |= D3D10_SHADER_PACK_MATRIX_ROW_MAJOR;

		HRESULT hr = ::D3DCompileFromFile(fileName.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, NULL, "fx_5_0", shaderFlags, NULL, blob.GetAddressOf(), error.GetAddressOf());
		if (FAILED(hr))
		{
			if (error != NULL)
			{
				string str = (const char*)error->GetBufferPointer();
				MessageBoxA(NULL, str.c_str(), "Shader Error", MB_OK);
			}
			assert(false);
		}

		ComPtr<ID3DX11Effect> effect;
		hr = ::D3DX11CreateEffectFromMemory(blob->GetBufferPointer(), blob->GetBufferSize(), 0, DEVICE.Get(), effect.GetAddressOf());
		CHECK(hr);

		shaders[fileName] = ShaderDesc{ blob, effect };
	}

	ShaderDesc desc = shaders.at(fileName);
	ComPtr<ID3DX11Effect> effect;
	desc.effect->CloneEffect(D3DX11_EFFECT_CLONE_FORCE_NONSINGLE, effect.GetAddressOf());

	return ShaderDesc{ desc.blob, effect };
}

void Shader::PushGlobalData(const Matrix& view, const Matrix& projection)
{
	if (_globalEffectBuffer == nullptr)
	{
		_globalBuffer = make_shared<ConstantBuffer<GlobalDesc>>();
		_globalBuffer->Create();
		_globalEffectBuffer = GetConstantBuffer("GlobalBuffer");
	}

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	Matrix toTexSpace(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	_globalDesc.V = view;
	_globalDesc.P = projection;
	_globalDesc.VP = view * projection;
	_globalDesc.VPT = _globalDesc.VP * toTexSpace;
	_globalDesc.VInv = view.Invert();
	_globalDesc.CamPos = Camera::S_Pos;

	_globalBuffer->CopyData(_globalDesc);
	_globalEffectBuffer->SetConstantBuffer(_globalBuffer->GetComPtr().Get());
}

void Shader::PushTransformData(const TransformDesc& desc)
{
	if (_transformEffectBuffer == nullptr)
	{
		_transformBuffer = make_shared<ConstantBuffer<TransformDesc>>();
		_transformBuffer->Create();
		_transformEffectBuffer = GetConstantBuffer("TransformBuffer");
	}

	_transformDesc = desc;
	_transformBuffer->CopyData(_transformDesc);
	_transformEffectBuffer->SetConstantBuffer(_transformBuffer->GetComPtr().Get());
}

void Shader::PushLightData(const LightDesc& desc)
{
	if (_lightEffectBuffer == nullptr)
	{
		_lightBuffer = make_shared<ConstantBuffer<LightDesc>>();
		_lightBuffer->Create();
		_lightEffectBuffer = GetConstantBuffer("LightBuffer");
	}

	_lightDesc = desc;
	_lightBuffer->CopyData(_lightDesc);
	_lightEffectBuffer->SetConstantBuffer(_lightBuffer->GetComPtr().Get());
}

void Shader::PushMaterialData(const MaterialDesc& desc)
{
	if (_materialEffectBuffer == nullptr)
	{
		_materialBuffer = make_shared<ConstantBuffer<MaterialDesc>>();
		_materialBuffer->Create();
		_materialEffectBuffer = GetConstantBuffer("MaterialBuffer");
	}

	_materialDesc = desc;
	_materialBuffer->CopyData(_materialDesc);
	_materialEffectBuffer->SetConstantBuffer(_materialBuffer->GetComPtr().Get());
}

void Shader::PushBoneData(const BoneDesc& desc)
{
	if (_boneEffectBuffer == nullptr)
	{
		_boneBuffer = make_shared<ConstantBuffer<BoneDesc>>();
		_boneBuffer->Create();
		_boneEffectBuffer = GetConstantBuffer("BoneBuffer");
	}

	_boneDesc = desc;
	_boneBuffer->CopyData(_boneDesc);
	_boneEffectBuffer->SetConstantBuffer(_boneBuffer->GetComPtr().Get());
}

void Shader::PushKeyframeData(const KeyframeDesc& desc)
{
	if (_keyframeEffectBuffer == nullptr)
	{
		_keyframeBuffer = make_shared<ConstantBuffer<KeyframeDesc>>();
		_keyframeBuffer->Create();
		_keyframeEffectBuffer = GetConstantBuffer("KeyframeBuffer");
	}

	_keyframeDesc = desc;
	_keyframeBuffer->CopyData(_keyframeDesc);
	_keyframeEffectBuffer->SetConstantBuffer(_keyframeBuffer->GetComPtr().Get());
}

void Shader::PushTweenData(const InstancedTweenDesc& desc)
{
	if (_tweenEffectBuffer == nullptr)
	{
		_tweenBuffer = make_shared<ConstantBuffer<InstancedTweenDesc>>();
		_tweenBuffer->Create();
		_tweenEffectBuffer = GetConstantBuffer("TweenBuffer");
	}

	_tweenDesc = desc;
	_tweenBuffer->CopyData(_tweenDesc);
	_tweenEffectBuffer->SetConstantBuffer(_tweenBuffer->GetComPtr().Get());
}

void Shader::PushSnowData(const SnowBillboardDesc& desc)
{
	if (_snowEffectBuffer == nullptr)
	{
		_snowBuffer = make_shared<ConstantBuffer<SnowBillboardDesc>>();
		_snowBuffer->Create();
		_snowEffectBuffer = GetConstantBuffer("SnowBuffer");
	}

	_snowDesc = desc;
	_snowBuffer->CopyData(_snowDesc);
	_snowEffectBuffer->SetConstantBuffer(_snowBuffer->GetComPtr().Get());
}

void Shader::PushParticleData(const ParticleDesc& desc)
{
	if (_particleEffectBuffer == nullptr)
	{
		_particleBuffer = make_shared<ConstantBuffer<ParticleDesc>>();
		_particleBuffer->Create();
		_particleEffectBuffer = GetConstantBuffer("ParticleBuffer");
	}

	_particleDesc = desc;
	_particleBuffer->CopyData(_particleDesc);
	_particleEffectBuffer->SetConstantBuffer(_particleBuffer->GetComPtr().Get());
}

void Shader::PushShadowData(const Matrix& desc)
{
	if (_shadowEffectBuffer == nullptr)
	{
		_shadowBuffer = make_shared<ConstantBuffer<Matrix>>();
		_shadowBuffer->Create();
		_shadowEffectBuffer = GetConstantBuffer("ShadowBuffer");
	}
	_shadowDesc = desc;
	_shadowBuffer->CopyData(_shadowDesc);
	_shadowEffectBuffer->SetConstantBuffer(_shadowBuffer->GetComPtr().Get());
}

void Shader::PushSsaoData(const SsaoDesc& desc)
{
	if (_ssaoEffectBuffer == nullptr)
	{
		_ssaoBuffer = make_shared<ConstantBuffer<SsaoDesc>>();
		_ssaoBuffer->Create();
		_ssaoEffectBuffer = GetConstantBuffer("SsaoBuffer");
	}
	_ssaoDesc = desc;
	_ssaoBuffer->CopyData(_ssaoDesc);
	_ssaoEffectBuffer->SetConstantBuffer(_ssaoBuffer->GetComPtr().Get());
}

void Shader::PushBlurData(const BlurDesc& desc)
{
	if (_blurEffectBuffer == nullptr)
	{
		_blurBuffer = make_shared<ConstantBuffer<BlurDesc>>();
		_blurBuffer->Create();
		_blurEffectBuffer = GetConstantBuffer("BlurBuffer");
	}
	_blurDesc = desc;
	_blurBuffer->CopyData(_blurDesc);
	_blurEffectBuffer->SetConstantBuffer(_blurBuffer->GetComPtr().Get());
}

int Shader::GetTechNum(RenderTech renderTech)
{ 
	int num = _techNums[static_cast<int>(renderTech)];
	assert(num != -1);
	return num;
}