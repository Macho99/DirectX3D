#include "pch.h"
#include "Ssao.h"
#include "MathUtils.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Material.h"
#include "Camera.h"

Ssao::Ssao()
{
    BuildQuad();
	BuildOffsetVectors();

	_texture0 = make_shared<Texture>();
	_texture1 = make_shared<Texture>();
}

Ssao::~Ssao()
{
}

void Ssao::OnSize(int32 width, int32 height, float fovy, float farZ)
{
	_renderTargetWidth = width;
	_renderTargetHeight = height;

	// We render to ambient map at half the resolution.
	_ambientMapViewport.Set(width * 0.5f, height * 0.5f);

	BuildFrustumCorners(fovy, farZ);
	BuildTextureViews();

	_blurDesc.gTexelHeight = 1.0f / _ambientMapViewport.GetHeight();
	_blurDesc.gTexelWidth = 1.0f / _ambientMapViewport.GetWidth();

	if (_ssaoMaterial == nullptr || _blurMaterial == nullptr)
	{
		{
			shared_ptr<Shader> shader = make_shared<Shader>(L"Ssao.fx");
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(shader);
			material->SetRandomTex(RESOURCES->Get<Texture>(L"RandomTex"));
			material->SetNormalMap(GRAPHICS->GetNormalDepthMap());
			_ssaoMaterial = material;
		}

		{
			shared_ptr<Shader> shader = make_shared<Shader>(L"Blur.fx");
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(shader);
			material->SetNormalMap(GRAPHICS->GetNormalDepthMap());
			_blurMaterial = material;
		}
	}
}

void Ssao::Clear()
{
	DC->ClearRenderTargetView(_ambientRTV0.Get(), reinterpret_cast<const float*>(&Colors::White));
}

void Ssao::Draw()
{
	ID3D11RenderTargetView* renderTargets[1] = { _ambientRTV0.Get() };
	DC->OMSetRenderTargets(1, renderTargets, 0);
	//DC->ClearRenderTargetView(_ambientRTV0.Get(), reinterpret_cast<const float*>(&Colors::Black));
	_ambientMapViewport.RSSetViewport();

	Shader* shader = _ssaoMaterial->GetShader();
	shader->PushSsaoData(_ssaoDesc);
	_ssaoMaterial->Update();

	static const Matrix T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	Matrix P = Camera::S_MatProjection;
	Matrix PT = P * T;
	_ssaoDesc.viewToTexSpace = PT;

	_screenQuadVB->PushData();
	_screenQuadIB->PushData();
	shader->DrawIndexed(RenderTech::Draw, 0, _screenQuadIB->GetCount());

	BlurAmbientMap(4);
}

void Ssao::BuildQuad()
{
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

void Ssao::BuildFrustumCorners(float fovy, float farZ)
{
	float aspect = (float)_renderTargetWidth / (float)_renderTargetHeight;

	float halfHeight = farZ * tanf(0.5f * fovy);
	float halfWidth = aspect * halfHeight;

	_ssaoDesc.frustumCorners[0] = Vec4(-halfWidth, -halfHeight, farZ, 0.0f);
	_ssaoDesc.frustumCorners[1] = Vec4(-halfWidth, +halfHeight, farZ, 0.0f);
	_ssaoDesc.frustumCorners[2] = Vec4(+halfWidth, +halfHeight, farZ, 0.0f);
	_ssaoDesc.frustumCorners[3] = Vec4(+halfWidth, -halfHeight, farZ, 0.0f);
}

void Ssao::BuildTextureViews()
{
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = _renderTargetWidth;
	texDesc.Height = _renderTargetHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ComPtr<ID3D11Texture2D> normalDepthTex;
	HR(DEVICE->CreateTexture2D(&texDesc, 0, normalDepthTex.GetAddressOf()));
	HR(DEVICE->CreateShaderResourceView(normalDepthTex.Get(), 0, _normalDepthSRV.GetAddressOf()));
	HR(DEVICE->CreateRenderTargetView(normalDepthTex.Get(), 0, _normalDepthRTV.GetAddressOf()));

	// Render ambient map at half resolution.
	texDesc.Width = _renderTargetWidth / 2;
	texDesc.Height = _renderTargetHeight / 2;
	texDesc.Format = DXGI_FORMAT_R16_FLOAT;

	ComPtr<ID3D11Texture2D> ambientTex0;
	HR(DEVICE->CreateTexture2D(&texDesc, 0, ambientTex0.GetAddressOf()));
	HR(DEVICE->CreateShaderResourceView(ambientTex0.Get(), 0, _ambientSRV0.GetAddressOf()));
	HR(DEVICE->CreateRenderTargetView(ambientTex0.Get(), 0, _ambientRTV0.GetAddressOf()));
	_texture0->SetSRV(_ambientSRV0);

	ComPtr<ID3D11Texture2D> ambientTex1;
	HR(DEVICE->CreateTexture2D(&texDesc, 0, ambientTex1.GetAddressOf()));
	HR(DEVICE->CreateShaderResourceView(ambientTex1.Get(), 0, _ambientSRV1.GetAddressOf()));
	HR(DEVICE->CreateRenderTargetView(ambientTex1.Get(), 0, _ambientRTV1.GetAddressOf()));
	_texture1->SetSRV(_ambientSRV1);
}

void Ssao::BuildOffsetVectors()
{
	// Start with 14 uniformly distributed vectors.  We choose the 8 corners of the cube
	// and the 6 center points along each cube face.  We always alternate the points on 
	// opposites sides of the cubes.  This way we still get the vectors spread out even
	// if we choose to use less than 14 samples.

	// 8 cube corners
	_ssaoDesc.offsetVectors[0] = Vec4(+1.0f, +1.0f, +1.0f, 0.0f);
	_ssaoDesc.offsetVectors[1] = Vec4(-1.0f, -1.0f, -1.0f, 0.0f);

	_ssaoDesc.offsetVectors[2] = Vec4(-1.0f, +1.0f, +1.0f, 0.0f);
	_ssaoDesc.offsetVectors[3] = Vec4(+1.0f, -1.0f, -1.0f, 0.0f);

	_ssaoDesc.offsetVectors[4] = Vec4(+1.0f, +1.0f, -1.0f, 0.0f);
	_ssaoDesc.offsetVectors[5] = Vec4(-1.0f, -1.0f, +1.0f, 0.0f);

	_ssaoDesc.offsetVectors[6] = Vec4(-1.0f, +1.0f, -1.0f, 0.0f);
	_ssaoDesc.offsetVectors[7] = Vec4(+1.0f, -1.0f, +1.0f, 0.0f);

	// 6 centers of cube faces
	_ssaoDesc.offsetVectors[8] = Vec4(-1.0f, 0.0f, 0.0f, 0.0f);
	_ssaoDesc.offsetVectors[9] = Vec4(+1.0f, 0.0f, 0.0f, 0.0f);

	_ssaoDesc.offsetVectors[10] = Vec4(0.0f, -1.0f, 0.0f, 0.0f);
	_ssaoDesc.offsetVectors[11] = Vec4(0.0f, +1.0f, 0.0f, 0.0f);

	_ssaoDesc.offsetVectors[12] = Vec4(0.0f, 0.0f, -1.0f, 0.0f);
	_ssaoDesc.offsetVectors[13] = Vec4(0.0f, 0.0f, +1.0f, 0.0f);

	for (int32 i = 0; i < 14; ++i)
	{
		// Create random lengths in [0.25, 1.0].
		float s = MathUtils::Random(0.25f, 1.0f);
		_ssaoDesc.offsetVectors[i].Normalize();
		_ssaoDesc.offsetVectors[i] *= s;
	}
}

void Ssao::BlurAmbientMap(int32 blurCount)
{
	for (int32 i = 0; i < blurCount; ++i)
	{
		// Ping-pong the two ambient map textures as we apply
		// horizontal and vertical blur passes.
		BlurAmbientMap(_texture0, _ambientRTV1, true);
		BlurAmbientMap(_texture1, _ambientRTV0, false);
	}
}

void Ssao::BlurAmbientMap(shared_ptr<Texture> inputTexture, ComPtr<ID3D11RenderTargetView> outputRTV, bool horzBlur)
{
	ID3D11RenderTargetView* renderTargets[1] = { outputRTV.Get() };
	DC->OMSetRenderTargets(1, renderTargets, 0);
	DC->ClearRenderTargetView(outputRTV.Get(), reinterpret_cast<const float*>(&Colors::Black));
	_ambientMapViewport.RSSetViewport();

	Shader* shader = _blurMaterial->GetShader();
	shader->PushBlurData(_blurDesc);
	//Effects::SsaoBlurFX->SetNormalDepthMap(_normalDepthSRV.Get());
	//Effects::SsaoBlurFX->SetInputImage(inputSRV.Get());
	_blurMaterial->SetDiffuseMap(inputTexture);
	_blurMaterial->Update();

	_screenQuadVB->PushData();
	_screenQuadIB->PushData();

	shader->DrawIndexed(horzBlur ? 0 : 1, 0, _screenQuadIB->GetCount());
}

void Ssao::SetNormalDepthRenderTarget(ID3D11DepthStencilView* dsv)
{
	ID3D11RenderTargetView* renderTargets[1] = { _normalDepthRTV.Get() };
	DC->OMSetRenderTargets(1, renderTargets, dsv);

	// Clear view space normal to (0,0,-1) and clear depth to be very far away.  
	float clearColor[] = { 0.0f, 0.0f, -1.0f, 1e5f };
	DC->ClearRenderTargetView(_normalDepthRTV.Get(), clearColor);
}
