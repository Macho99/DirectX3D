#pragma once

#include "Viewport.h"
class Ssao;
class PostProcess;

#define NUM_SHADOW_CASCADES 3
#define FRUSTUM_CORNERS 8

class Graphics
{
	DECLARE_SINGLE(Graphics);

public:
	void Init(HWND hwnd);
    void OnDestroy();

	void OnSize(bool isFirst);

	void RenderBegin();
	void RenderEnd();

	ComPtr<ID3D11Device> GetDevice() { return _device; }
	ComPtr<ID3D11DeviceContext> GetDeviceContext() { return _deviceContext; }

	void ClearShaderResources();

	void ClearDepthStencilView();

	void ClearShadowDepthStencilView(int index);
	void SetShadowDepthStencilView(int index);

	void SetNormalDepthRenderTarget();
	void DrawSsaoMap(bool clearOnly);
	shared_ptr<Texture> GetNormalDepthMap();
	shared_ptr<Texture> GetSsaoMap();
	void SetRTVAndDSV();
    void SetBackBufferRenderTarget();

	void DrawPostProcesses();

private:
	void CreateDeviceAndSwapChain();
	void CreateRenderTargetView();
	void CreateDSVAndShadowMap(bool createShadowMap);

public:
	void SetViewport(float width, float height, float x = 0, float y = 0, float minDepth = 0, float maxDepth = 1);
	Viewport& GetViewport() { return _vp; }
	Viewport& GetShadowViewport() { return _shadowVP; }
    ComPtr<ID3D11ShaderResourceView> GetSceneViewSRV() { return _sceneSRV; }
    ComPtr<ID3D11ShaderResourceView> GetShadowArraySRV() { return _shadowArraySRV; }
	shared_ptr<Texture> GetShadowMap(int index) { return _shadowMap[index]; }
    shared_ptr<Texture> GetPostProcessDebugTexture(int index) { return _ppDebugTextures[index]; }

public:
    float GetCascadeEnd(int index) const { return _cascadeEnds[index]; }
    const float* GetCascadeEnds() { return _cascadeEnds; }
    const Vec3* GetFrustumCornerNDC() { return _frustumCornersNDC; }

private:
	float _cascadeEnds[NUM_SHADOW_CASCADES + 1];
	// NDC Space Unit Cube (DX11 ±‚¡ÿ)
	Vec3 _frustumCornersNDC[FRUSTUM_CORNERS] =
	{
		Vec3(-1.0f,  1.0f, 0.0f), // Near
		Vec3( 1.0f,  1.0f, 0.0f),
		Vec3( 1.0f, -1.0f, 0.0f),
		Vec3(-1.0f, -1.0f, 0.0f),

		Vec3(-1.0f,  1.0f, 1.0f), // Far
		Vec3( 1.0f,  1.0f, 1.0f),
		Vec3( 1.0f, -1.0f, 1.0f),
		Vec3(-1.0f, -1.0f, 1.0f),
	};

private:
	HWND _hwnd = {};

	// Device & SwapChain
	ComPtr<ID3D11Device> _device = nullptr;
	ComPtr<ID3D11DeviceContext> _deviceContext = nullptr;
	ComPtr<IDXGISwapChain> _swapChain = nullptr;

	// RTV
	ComPtr<ID3D11Texture2D> _backBufferTexture;
	ComPtr<ID3D11RenderTargetView> _renderTargetView;
    ComPtr<ID3D11ShaderResourceView> _backBufferSRV;

	ComPtr<ID3D11Texture2D> _sceneTexture;
	ComPtr<ID3D11RenderTargetView> _sceneRTV;
	ComPtr<ID3D11ShaderResourceView> _sceneSRV;

	// DSV
	ComPtr<ID3D11Texture2D> _depthStencilTexture;
	ComPtr<ID3D11DepthStencilView> _depthStencilView;

	ComPtr<ID3D11Texture2D> _shadowDSTexture;
	ComPtr<ID3D11DepthStencilView> _shadowDSV[NUM_SHADOW_CASCADES];
	ComPtr<ID3D11ShaderResourceView> _shadowArraySRV;
	shared_ptr<Texture> _shadowMap[NUM_SHADOW_CASCADES];

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

