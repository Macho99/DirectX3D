#pragma once

#include "Viewport.h"

class Graphics
{
	DECLARE_SINGLE(Graphics);

public:
	void Init(HWND hwnd);

	void RenderBegin();
	void RenderEnd();

	ComPtr<ID3D11Device> GetDevice() { return _device; }
	ComPtr<ID3D11DeviceContext> GetDeviceContext() { return _deviceContext; }

	void ClearDepthStencilView();
	void ClearShadowDepthStencilView();
	void SetShadowDepthStencilView();
	void SetRTVAndDSV();

private:
	void CreateDeviceAndSwapChain();
	void CreateRenderTargetView();
	void CreateDepthStencilView();

public:
	void SetViewport(float width, float height, float x = 0, float y = 0, float minDepth = 0, float maxDepth = 1);
	Viewport& GetViewport() { return _vp; }
	Viewport& GetShadowViewport() { return _shadowVP; }

	//ComPtr<ID3D11ShaderResourceView> GetShadowMapSRV() { return _shadowSRV; }
	shared_ptr<Texture> GetShadowMap() { return _shadowMap; }

private:
	HWND _hwnd = {};

	// Device & SwapChain
	ComPtr<ID3D11Device> _device = nullptr;
	ComPtr<ID3D11DeviceContext> _deviceContext = nullptr;
	ComPtr<IDXGISwapChain> _swapChain = nullptr;

	// RTV
	ComPtr<ID3D11RenderTargetView> _renderTargetView;

	// DSV
	ComPtr<ID3D11Texture2D> _depthStencilTexture;
	ComPtr<ID3D11DepthStencilView> _depthStencilView;
	ComPtr<ID3D11Texture2D> _shadowDSTexture;
	ComPtr<ID3D11DepthStencilView> _shadowDSV;
	//ComPtr<ID3D11ShaderResourceView> _shadowSRV;
	shared_ptr<Texture> _shadowMap;

	// Misc
	//D3D11_VIEWPORT _viewport = { 0 };
	Viewport _vp;
	Viewport _shadowVP;
};

