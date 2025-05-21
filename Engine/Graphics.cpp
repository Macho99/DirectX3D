#include "pch.h"
#include "Graphics.h"
#include "Viewport.h"

#define SHADOWMAP_SIZE 4096

void Graphics::Init(HWND hwnd)
{
	_hwnd = hwnd;

	CreateDeviceAndSwapChain();
	CreateRenderTargetView();
	CreateDepthStencilView();
	
	SetViewport(GAME->GetGameDesc().width, GAME->GetGameDesc().height);
}

void Graphics::RenderBegin()
{
	_deviceContext->OMSetRenderTargets(1, _renderTargetView.GetAddressOf(), _depthStencilView.Get());
	//ClearDepthStencilView();
	//씬에서 카메라마다 클리어해줌
	_deviceContext->ClearRenderTargetView(_renderTargetView.Get(), (float*)(&GAME->GetGameDesc().clearColor));
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

void Graphics::SetRTVAndDSV()
{
	_vp.RSSetViewport();
	_deviceContext->OMSetRenderTargets(1, _renderTargetView.GetAddressOf(), _depthStencilView.Get());
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

	ComPtr<ID3D11Texture2D> backBuffer = nullptr;
	hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf());
	CHECK(hr);

	hr = _device->CreateRenderTargetView(backBuffer.Get(), nullptr, _renderTargetView.GetAddressOf());
	CHECK(hr);
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
