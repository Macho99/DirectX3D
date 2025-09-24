#pragma once

#include "Viewport.h"
class Ssao;
class PostProcess;

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

	void SetNormalDepthRenderTarget();
	void DrawSsaoMap(bool clearOnly);
	shared_ptr<Texture> GetNormalDepthMap();
	shared_ptr<Texture> GetSsaoMap();
	void SetRTVAndDSV();

	void SetSsaoSize(int32 width, int32 height, float fovy, float farZ);

	void DrawPostProcesses();

private:
	void CreateDeviceAndSwapChain();
	void CreateRenderTargetView();
	void CreateDepthStencilView();

public:
	void SetViewport(float width, float height, float x = 0, float y = 0, float minDepth = 0, float maxDepth = 1);
	Viewport& GetViewport() { return _vp; }
	Viewport& GetShadowViewport() { return _shadowVP; }
	shared_ptr<Texture> GetShadowMap() { return _shadowMap; }
    shared_ptr<Texture> GetPostProcessDebugTexture(int index) { return _ppDebugTextures[index]; }

private:
	HWND _hwnd = {};

	// Device & SwapChain
	ComPtr<ID3D11Device> _device = nullptr;
	ComPtr<ID3D11DeviceContext> _deviceContext = nullptr;
	ComPtr<IDXGISwapChain> _swapChain = nullptr;

	// RTV
	ComPtr<ID3D11Texture2D> _backBufferTexture;
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

	shared_ptr<Ssao> _ssao;
	shared_ptr<Texture> _normalDepthMap;

    vector<shared_ptr<PostProcess>> _postProcesses;
    vector<shared_ptr<Texture>> _ppDebugTextures;
	vector<ComPtr<ID3D11Texture2D>> _ppTextures;
    vector<ComPtr<ID3D11ShaderResourceView>> _ppSRVs;
    vector<ComPtr<ID3D11RenderTargetView>> _ppRTVs;

    ComPtr<ID3D11Texture2D> _hdrTexture;
	ComPtr<ID3D11ShaderResourceView> _hdrSRV;
	ComPtr<ID3D11RenderTargetView> _hdrRTV;
};

