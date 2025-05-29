#include "pch.h"
#include "Ssao.h"
#include "MathUtils.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

Ssao::Ssao()
{
    BuildQuad();
	BuildOffsetVectors();
}

Ssao::~Ssao()
{
}

void Ssao::OnSize(int32 width, int32 height, float fovy, float farZ)
{
	_renderTargetWidth = width;
	_renderTargetHeight = height;

	// We render to ambient map at half the resolution.
	_ambientMapViewport.Set(width, height);

	BuildFrustumCorners(fovy, farZ);
	BuildTextureViews();
}

void Ssao::BuildQuad()
{
	vector<VertexTextureNormalData> v(4);

	v[0].position = XMFLOAT3(-1.0f, -1.0f, 0.0f);
	v[1].position = XMFLOAT3(-1.0f, +1.0f, 0.0f);
	v[2].position = XMFLOAT3(+1.0f, +1.0f, 0.0f);
	v[3].position = XMFLOAT3(+1.0f, -1.0f, 0.0f);

	// Stnre far plane frustum corner indices in Normal.x slot.
	v[0].normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	v[1].normal = XMFLOAT3(1.0f, 0.0f, 0.0f);
	v[2].normal = XMFLOAT3(2.0f, 0.0f, 0.0f);
	v[3].normal = XMFLOAT3(3.0f, 0.0f, 0.0f);

	v[0].uv = XMFLOAT2(0.0f, 1.0f);
	v[1].uv = XMFLOAT2(0.0f, 0.0f);
	v[2].uv = XMFLOAT2(1.0f, 0.0f);
	v[3].uv = XMFLOAT2(1.0f, 1.0f);

    _screenQuadVB = make_unique<VertexBuffer>();
    _screenQuadVB->Create(v, 0);

	vector<uint32> indices = { 0, 1, 2, 0, 2, 3 };

    _screenQuadIB = make_unique<IndexBuffer>();
    _screenQuadIB->Create(indices);
}

void Ssao::BuildFrustumCorners(float fovy, float farZ)
{
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

	ComPtr<ID3D11Texture2D> ambientTex1;
	HR(DEVICE->CreateTexture2D(&texDesc, 0, ambientTex1.GetAddressOf()));
	HR(DEVICE->CreateShaderResourceView(ambientTex1.Get(), 0, _ambientSRV1.GetAddressOf()));
	HR(DEVICE->CreateRenderTargetView(ambientTex1.Get(), 0, _ambientRTV1.GetAddressOf()));
}

void Ssao::BuildOffsetVectors()
{
	// Start with 14 uniformly distributed vectors.  We choose the 8 corners of the cube
	// and the 6 center points along each cube face.  We always alternate the points on 
	// opposites sides of the cubes.  This way we still get the vectors spread out even
	// if we choose to use less than 14 samples.

	// 8 cube corners
	_offsets[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
	_offsets[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

	_offsets[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
	_offsets[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

	_offsets[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
	_offsets[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

	_offsets[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
	_offsets[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

	// 6 centers of cube faces
	_offsets[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
	_offsets[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

	_offsets[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	_offsets[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

	_offsets[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
	_offsets[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

	for (int32 i = 0; i < 14; ++i)
	{
		// Create random lengths in [0.25, 1.0].
		float s = MathUtils::Random(0.25f, 1.0f);
		_offsets[i].Normalize();
        _offsets[i] *= s;
	}
}

void Ssao::SetNormalDepthRenderTarget(ID3D11DepthStencilView* dsv)
{
	ID3D11RenderTargetView* renderTargets[1] = { _normalDepthRTV.Get() };
	DC->OMSetRenderTargets(1, renderTargets, dsv);

	// Clear view space normal to (0,0,-1) and clear depth to be very far away.  
	float clearColor[] = { 0.0f, 0.0f, -1.0f, 1e5f };
	DC->ClearRenderTargetView(_normalDepthRTV.Get(), clearColor);
}
