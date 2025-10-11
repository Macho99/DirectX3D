#include "pch.h"
#include "Graphics.h"
#include "Ssao.h"
#include "Viewport.h"
#include "Bloom.h"
#include "ToneMapping.h"

#define SHADOWMAP_SIZE 4096

void Graphics::Init(HWND hwnd)
{
	_hwnd = hwnd;

	CreateDeviceAndSwapChain();

	_postProcesses.push_back(make_shared<Bloom>());
	//_postProcesses[0]->SetEnabled(false);
    _postProcesses.push_back(make_shared<ToneMapping>());

	CreateRenderTargetView();
	CreateDepthStencilView();
	
	SetViewport(GAME->GetGameDesc().width, GAME->GetGameDesc().height);
	_ssao = make_shared<Ssao>();
	_normalDepthMap = make_shared<Texture>();
}

void Graphics::RenderBegin()
{
	_deviceContext->OMSetRenderTargets(1, _hdrRTV.GetAddressOf(), _depthStencilView.Get());
	//ClearDepthStencilView();
	//씬에서 카메라마다 클리어해줌
	_deviceContext->ClearRenderTargetView(_hdrRTV.Get(), (float*)(&GAME->GetGameDesc().clearColor));
	_vp.RSSetViewport();
}

void Graphics::RenderEnd()
{
	HRESULT hr = _swapChain->Present(0, 0);
	CHECK(hr);
}

void Graphics::ClearDepthStencilView()
{	
	_deviceContext->ClearDepthStencilView(_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1, 0);
}

void Graphics::ClearShadowDepthStencilView()
{
	_deviceContext->ClearDepthStencilView(_shadowDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1, 0);
}

void Graphics::SetShadowDepthStencilView()
{
	_shadowVP.RSSetViewport();

	ID3D11RenderTargetView* renderTargets[1] = { 0 };
	_deviceContext->OMSetRenderTargets(1, renderTargets, _shadowDSV.Get());
}

void Graphics::SetNormalDepthRenderTarget()
{
	_vp.RSSetViewport();
	_ssao->SetNormalDepthRenderTarget(_depthStencilView.Get());
}

void Graphics::DrawSsaoMap(bool clearOnly)
{
	_ssao->Clear();
	if(clearOnly == false)
		_ssao->Draw();
}

shared_ptr<Texture> Graphics::GetNormalDepthMap()
{
	return _normalDepthMap;
}

shared_ptr<Texture> Graphics::GetSsaoMap()
{
	return _ssao->GetSsaoMap();
}

void Graphics::SetRTVAndDSV()
{
	_vp.RSSetViewport();
	_deviceContext->OMSetRenderTargets(1, _hdrRTV.GetAddressOf(), _depthStencilView.Get());
}

void Graphics::SetSsaoSize(int32 width, int32 height, float fovy, float farZ)
{
	_ssao->OnSize(width, height, fovy, farZ);
	_normalDepthMap->SetSRV(_ssao->GetNormalDepthSRV());
}

void Graphics::DrawPostProcesses()
{
	for (int i = 0; i < _postProcesses.size() - 1; i++)
	{
		PostProcess* postProcess = _postProcesses[i].get();
        if (postProcess->IsEnabled() == false)
            continue;
		postProcess->Render(_hdrSRV, _ppRTVs[i]);
		_ppDebugTextures[i]->SetSRV(_ppSRVs[i]);
		_deviceContext->CopyResource(_hdrTexture.Get(), _ppTextures[i].Get());
        postProcess->SetDebugTextureSRV(_ppDebugTextures[i]);
		_vp.RSSetViewport();
	}

    _deviceContext->OMSetRenderTargets(1, _renderTargetView.GetAddressOf(), 0);

    PostProcess* toneMapping = _postProcesses.back().get();
    if (toneMapping->IsEnabled())
        toneMapping->Render(_hdrSRV, _renderTargetView);
}

void Graphics::CreateDeviceAndSwapChain()
{
	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	{
		desc.BufferDesc.Width = GAME->GetGameDesc().width;
		desc.BufferDesc.Height = GAME->GetGameDesc().height;
		desc.BufferDesc.RefreshRate.Numerator = 60;
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = 1;
		desc.OutputWindow = _hwnd;
		desc.Windowed = TRUE;
		desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	}

	HRESULT hr = ::D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		D3D11_CREATE_DEVICE_DEBUG,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&desc,
		_swapChain.GetAddressOf(),
		_device.GetAddressOf(),
		nullptr,
		_deviceContext.GetAddressOf()
	);

	CHECK(hr);
}

void Graphics::CreateRenderTargetView()
{
	HRESULT hr;

	//ComPtr<ID3D11Texture2D> backBuffer = nullptr;
	hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)_backBufferTexture.GetAddressOf());
	CHECK(hr);

	hr = _device->CreateRenderTargetView(_backBufferTexture.Get(), nullptr, _renderTargetView.GetAddressOf());
	CHECK(hr);

	{
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = static_cast<uint32>(GAME->GetGameDesc().width);
		texDesc.Height = static_cast<uint32>(GAME->GetGameDesc().height);
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		HR(DEVICE->CreateTexture2D(&texDesc, 0, _hdrTexture.GetAddressOf()));
		HR(DEVICE->CreateShaderResourceView(_hdrTexture.Get(), 0, _hdrSRV.GetAddressOf()));
		HR(DEVICE->CreateRenderTargetView(_hdrTexture.Get(), 0, _hdrRTV.GetAddressOf()));

		for (int i = 0; i < _postProcesses.size() - 1; i++)
		{
            ComPtr<ID3D11Texture2D> ppTex;
            ComPtr<ID3D11ShaderResourceView> ppSRV;
            ComPtr<ID3D11RenderTargetView> ppRTV;

			HR(DEVICE->CreateTexture2D(&texDesc, 0, ppTex.GetAddressOf()));
			HR(DEVICE->CreateShaderResourceView(ppTex.Get(), 0, ppSRV.GetAddressOf()));
			HR(DEVICE->CreateRenderTargetView(ppTex.Get(), 0, ppRTV.GetAddressOf()));

            _ppTextures.push_back(ppTex);
            _ppSRVs.push_back(ppSRV);
            _ppRTVs.push_back(ppRTV);

            _ppDebugTextures.push_back(make_shared<Texture>());
		}
	}
}

void Graphics::CreateDepthStencilView()
{
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = static_cast<uint32>(GAME->GetGameDesc().width);
		desc.Height = static_cast<uint32>(GAME->GetGameDesc().height);
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		HRESULT hr = DEVICE->CreateTexture2D(&desc, nullptr, _depthStencilTexture.GetAddressOf());
		CHECK(hr);

		desc.Width = SHADOWMAP_SIZE;
		desc.Height = SHADOWMAP_SIZE;
		desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		hr = DEVICE->CreateTexture2D(&desc, nullptr, _shadowDSTexture.GetAddressOf());
		CHECK(hr);
	}

	{
		D3D11_DEPTH_STENCIL_VIEW_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Flags = 0;
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;

		HRESULT hr = DEVICE->CreateDepthStencilView(_depthStencilTexture.Get(), &desc, _depthStencilView.GetAddressOf());
		CHECK(hr);

		hr = DEVICE->CreateDepthStencilView(_shadowDSTexture.Get(), &desc, _shadowDSV.GetAddressOf());
		CHECK(hr);
	}

	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		ComPtr<ID3D11ShaderResourceView> srv;
		HRESULT hr = DEVICE->CreateShaderResourceView(_shadowDSTexture.Get(), &srvDesc, srv.GetAddressOf());
		CHECK(hr);

		_shadowMap = make_shared<Texture>();
		_shadowMap->SetSRV(srv);
	}
}

void Graphics::SetViewport(float width, float height, float x, float y, float minDepth, float maxDepth)
{
	_vp.Set(width, height, x, y, minDepth, maxDepth);
	_shadowVP.Set(SHADOWMAP_SIZE, SHADOWMAP_SIZE, x, y, minDepth, maxDepth);
}
