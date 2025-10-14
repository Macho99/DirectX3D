#include "pch.h"
#include "Blur.h"
#include "Material.h"

Blur::Blur()
{
	_texture0 = make_shared<Texture>();
	_texture1 = make_shared<Texture>();

	vector<VertexTextureNormalData> v(4);

	v[0].position = Vec3(-1.0f, -1.0f, 0.0f);
	v[1].position = Vec3(-1.0f, +1.0f, 0.0f);
	v[2].position = Vec3(+1.0f, +1.0f, 0.0f);
	v[3].position = Vec3(+1.0f, -1.0f, 0.0f);

	// Stnre far plane frustum corner indices in Normal.x slot.
	v[0].normal = Vec3(0.0f, 0.0f, 0.0f);
	v[1].normal = Vec3(1.0f, 0.0f, 0.0f);
	v[2].normal = Vec3(2.0f, 0.0f, 0.0f);
	v[3].normal = Vec3(3.0f, 0.0f, 0.0f);

	v[0].uv = Vec2(0.0f, 1.0f);
	v[1].uv = Vec2(0.0f, 0.0f);
	v[2].uv = Vec2(1.0f, 0.0f);
	v[3].uv = Vec2(1.0f, 1.0f);

	_screenQuadVB = make_unique<VertexBuffer>();
	_screenQuadVB->Create(v, 0);

	vector<uint32> indices = { 0, 1, 2, 0, 2, 3 };

	_screenQuadIB = make_unique<IndexBuffer>();
	_screenQuadIB->Create(indices);
}

Blur::~Blur()
{
}

void Blur::OnSize(int32 width, int32 height)
{
	_blurDesc.gTexelHeight = 1.0f / height;
	_blurDesc.gTexelWidth = 1.0f / width;

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	HR(DEVICE->CreateTexture2D(&texDesc, 0, _ambientTexture0.GetAddressOf()));
	HR(DEVICE->CreateShaderResourceView(_ambientTexture0.Get(), 0, _ambientSRV0.GetAddressOf()));
	HR(DEVICE->CreateRenderTargetView(_ambientTexture0.Get(), 0, _ambientRTV0.GetAddressOf()));
	_texture0->SetSRV(_ambientSRV0);

	HR(DEVICE->CreateTexture2D(&texDesc, 0, _ambientTexture1.GetAddressOf()));
	HR(DEVICE->CreateShaderResourceView(_ambientTexture1.Get(), 0, _ambientSRV1.GetAddressOf()));
	HR(DEVICE->CreateRenderTargetView(_ambientTexture1.Get(), 0, _ambientRTV1.GetAddressOf()));
	_texture1->SetSRV(_ambientSRV1);

	if (_blurMat == nullptr)
	{
		shared_ptr<Shader> shader = make_shared<Shader>(L"Blur.fx");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(shader);
		material->SetNormalMap(GRAPHICS->GetNormalDepthMap());
		_blurMat = material;
	}
}

void Blur::ProcessBlur(int32 blurCount)
{
	for (int32 i = 0; i < blurCount; ++i)
	{
		// Ping-pong the two ambient map textures as we apply
		// horizontal and vertical blur passes.
		ProcessBlur(_texture0, _ambientRTV1, true);
		ProcessBlur(_texture1, _ambientRTV0, false);
	}
}

void Blur::ProcessBlur(shared_ptr<Texture> inputTexture, ComPtr<ID3D11RenderTargetView> outputRTV, bool horzBlur)
{
	ID3D11RenderTargetView* renderTargets[1] = { outputRTV.Get() };
	DC->OMSetRenderTargets(1, renderTargets, 0);
	DC->ClearRenderTargetView(outputRTV.Get(), reinterpret_cast<const float*>(&Colors::Black));

	Shader* shader = _blurMat->GetShader();
	shader->PushBlurData(_blurDesc);
	//Effects::SsaoBlurFX->SetNormalDepthMap(_normalDepthSRV.Get());
	//Effects::SsaoBlurFX->SetInputImage(inputSRV.Get());
	_blurMat->SetDiffuseMap(inputTexture);
	_blurMat->Update();

	_screenQuadVB->PushData();
	_screenQuadIB->PushData();

	shader->DrawIndexed(horzBlur ? 0 : 1, 0, _screenQuadIB->GetCount());
}