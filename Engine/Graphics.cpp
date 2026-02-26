#include "pch.h"
#include "Graphics.h"
#include "Ssao.h"
#include "Viewport.h"
#include "Bloom.h"
#include "ToneMapping.h"
#include "Camera.h"

#define SHADOWMAP_SIZE 4096

void Graphics::Init(HWND hwnd)
{
	_cascadeEnds[0] = 0.0f;
	_cascadeEnds[1] = 0.05f;
	_cascadeEnds[2] = 0.2f;
	_cascadeEnds[3] = 1.0f;

	_hwnd = hwnd;

	CreateDeviceAndSwapChain();

	_postProcesses.push_back(make_shared<Bloom>());
	//_postProcesses[0]->SetEnabled(false);
    _postProcesses.push_back(make_shared<ToneMapping>());

	CreateRenderTargetView();
	CreateDSVAndShadowMap(true);
	
	_ssao = make_shared<Ssao>();
	_normalDepthMap = RESOURCES->AllocateTempResource(make_unique<Texture>());
}

void Graphics::OnDestroy()
{
	// =========================
	// PostProcess / 공유 객체
	// =========================
	_postProcesses.clear();
	_ppDebugTextures.clear();

	_ssao.reset();
	//_normalDepthMap.reset();

	//for (int i = 0; i < NUM_SHADOW_CASCADES; ++i)
	//{
	//	_shadowMap[i].reset();
	//}

	// =========================
	// Shadow resources
	// =========================
	_shadowArraySRV.Reset();

	for (int i = 0; i < NUM_SHADOW_CASCADES; ++i)
	{
		_shadowDSV[i].Reset();
	}

	_shadowDSTexture.Reset();

	// =========================
	// HDR
	// =========================
	_hdrRTV.Reset();
	_hdrSRV.Reset();
	_hdrTexture.Reset();
	
	// =========================
	// PostProcess buffers
	// =========================
	_ppRTVs.clear();
	_ppSRVs.clear();
	_ppTextures.clear();

	// =========================
	// Main RTV / DSV
	// =========================
	_renderTargetView.Reset();
	_depthStencilView.Reset();

	_backBufferTexture.Reset();
	_depthStencilTexture.Reset();

	// =========================
	// SwapChain
	// =========================
	if (_swapChain)
	{
		_swapChain->SetFullscreenState(FALSE, nullptr);
		_swapChain.Reset();
	}

	_deviceContext->ClearState();
	_deviceContext->Flush();
	_deviceContext.Reset();

	//ID3D11Debug* d3dDebug = nullptr; 
	//HRESULT hr = _device->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug);
	//if (SUCCEEDED(hr))
	//{
	//	OutputDebugStringW(L"==============출력 시작============\n");
	//	// D3D11_RLDO_DETAIL을 사용하면 어떤 객체(Texture, Buffer 등)가 남았는지 상세히 보여줍니다.
	//	d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	//	d3dDebug->Release();
	//	OutputDebugStringW(L"==============출력 종료============\n");
	//}

	_device.Reset();
	_hwnd = nullptr;
}

void Graphics::OnSize(bool isFirst)
{
	int width = GAME->GetGameDesc().width;
	int height = GAME->GetGameDesc().height;
    int sceneWidth = GAME->GetGameDesc().sceneWidth;
    int sceneHeight = GAME->GetGameDesc().sceneHeight;
	if (isFirst == false)
	{
		// 1) GPU 파이프라인에서 백버퍼/DS를 먼저 떼기
		ID3D11RenderTargetView* nullRTV[1] = { nullptr };
		_deviceContext->OMSetRenderTargets(1, nullRTV, nullptr);

		// (선택) 혹시 다른 곳에 바인딩된 SRV/UAV도 확실히 떼기
		ID3D11ShaderResourceView* nullSRV[16] = {};
		_deviceContext->PSSetShaderResources(0, 16, nullSRV);
		_deviceContext->CSSetShaderResources(0, 16, nullSRV);
		// UAV까지 쓰면 CSSetUnorderedAccessViews도 null로

		// (선택) 즉시 해제 큐 처리
		_deviceContext->Flush();

		// 2) 스왑체인 버퍼로부터 만든 리소스들 Release
		_renderTargetView.Reset();
		_backBufferTexture.Reset();
		// 백버퍼로 만든 SRV/기타도 Reset
		
        HRESULT hr = _swapChain->ResizeBuffers(0,
			width,
			height,
            DXGI_FORMAT_UNKNOWN,
            0);
        CHECK(hr);

        _swapChain->GetBuffer(0, IID_PPV_ARGS(&_backBufferTexture));
        CreateRenderTargetView();
		CreateDSVAndShadowMap(false);
	}

    auto& cameras = CUR_SCENE->GetCameras();
    for (auto& camera : cameras)
    {
        camera.Resolve()->GetCamera()->OnSize();
    }

	Camera* mainCam = CUR_SCENE->GetMainCamera()->GetCamera();
    float fov = mainCam->GetFOV();
    float farZ = mainCam->GetFar();
	_ssao->OnSize(sceneWidth, sceneHeight, fov, farZ);
	_normalDepthMap.Resolve()->SetSRV(_ssao->GetNormalDepthSRV());
    _normalDepthMap.Resolve()->SetSize(Vec2((float)sceneWidth, (float)sceneHeight));
	SetViewport(sceneWidth, sceneHeight);
    for (auto& postProcess : _postProcesses)
    {
        postProcess->OnSize(sceneWidth, sceneHeight);
    }
}

void Graphics::RenderBegin()
{
	ClearShaderResources();

	_deviceContext->OMSetRenderTargets(1, _hdrRTV.GetAddressOf(), _depthStencilView.Get());
	//ClearDepthStencilView();
	//씬에서 카메라마다 클리어해줌
	_deviceContext->ClearRenderTargetView(_hdrRTV.Get(), (float*)(&GAME->GetGameDesc().clearColor));
	_vp.RSSetViewport();
}

void Graphics::RenderEnd()
{
	HRESULT hr = _swapChain->Present(2, 0);
	CHECK(hr);
}

void Graphics::ClearShaderResources()
{
    ID3D11ShaderResourceView* nullSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
    _deviceContext->PSSetShaderResources(0, 4, nullSRVs);
    _deviceContext->VSSetShaderResources(0, 4, nullSRVs);
}

void Graphics::ClearDepthStencilView()
{	
	_deviceContext->ClearDepthStencilView(_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1, 0);
}

void Graphics::ClearShadowDepthStencilView(int index)
{
	_deviceContext->ClearDepthStencilView(_shadowDSV[index].Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1, 0);
}

void Graphics::SetShadowDepthStencilView(int index)
{
	_shadowVP.RSSetViewport();

	ID3D11RenderTargetView* renderTargets[1] = { 0 };
	_deviceContext->OMSetRenderTargets(1, renderTargets, _shadowDSV[index].Get());
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

ResourceRef<Texture> Graphics::GetNormalDepthMap()
{
	return _normalDepthMap;
}

ResourceRef<Texture> Graphics::GetSsaoMap()
{
	return _ssao->GetSsaoMap();
}

void Graphics::SetRTVAndDSV()
{
	_vp.RSSetViewport();
	_deviceContext->OMSetRenderTargets(1, _hdrRTV.GetAddressOf(), _depthStencilView.Get());
}

void Graphics::SetBackBufferRenderTarget()
{
    _deviceContext->OMSetRenderTargets(1, _renderTargetView.GetAddressOf(), nullptr);
}

void Graphics::DrawPostProcesses()
{
	if (INPUT->GetButtonDown(KEY_TYPE::KEY_1) && !INPUT->GetButton(KEY_TYPE::LSHIFT))
	{
        _postProcesses[0]->SetEnabled(!_postProcesses[0]->IsEnabled());
	}

	for (int i = 0; i < _postProcesses.size() - 1; i++)
	{
		PostProcess* postProcess = _postProcesses[i].get();
        if (postProcess->IsEnabled() == false)
            continue;
		postProcess->Render(_ppRTVs[i]);
		_deviceContext->CopyResource(_hdrTexture.Get(), _ppTextures[i].Get());
        postProcess->SetDebugTextureSRV(_ppDebugTextures[i]);
		_vp.RSSetViewport();
	}

	ComPtr<ID3D11RenderTargetView> rtv = GAME->GetGameDesc().isEditor ? _sceneRTV : _renderTargetView;
    _deviceContext->OMSetRenderTargets(1, rtv.GetAddressOf(), 0);

    PostProcess* toneMapping = _postProcesses.back().get();
    if (toneMapping->IsEnabled())
        toneMapping->Render(rtv);
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

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0
	};

	HRESULT hr = ::D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		D3D11_CREATE_DEVICE_DEBUG,
		featureLevels,
		1,
		D3D11_SDK_VERSION,
		&desc,
		_swapChain.GetAddressOf(),
		_device.GetAddressOf(),
		nullptr,
		_deviceContext.GetAddressOf()
	);
	CHECK(hr);

	ComPtr<ID3D11InfoQueue> infoQueue;
	if (SUCCEEDED(DEVICE->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&infoQueue)))
	{
		// 408을 “숨길” 필터
		D3D11_MESSAGE_ID hide[] = {
			(D3D11_MESSAGE_ID)408, // QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS
		};

		D3D11_INFO_QUEUE_FILTER f = {};
		f.DenyList.NumIDs = _countof(hide);
		f.DenyList.pIDList = hide;

		infoQueue->AddStorageFilterEntries(&f);

		// 경고(Warning)가 발생했을 때 브레이크포인트를 겁니다.
		infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);

		// 에러(Error)가 발생했을 때 브레이크포인트를 겁니다.
		infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);

		// 치명적인 오류(Corruption) 발생 시에도 걸고 싶다면
		infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	}
}

void Graphics::CreateRenderTargetView()
{
	HRESULT hr;
	_ppTextures.clear();
	_ppSRVs.clear();
	_ppRTVs.clear();
    _ppDebugTextures.clear();

    float sceneWidth = GAME->GetGameDesc().sceneWidth;
    float sceneHeight = GAME->GetGameDesc().sceneHeight;

    _backBufferTexture.Reset();
	CHECK(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)_backBufferTexture.GetAddressOf()));
	if (GAME->GetGameDesc().isEditor)
	{
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = static_cast<uint32>(sceneWidth);
		texDesc.Height = static_cast<uint32>(sceneHeight);
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;
		DX_CREATE_TEXTURE2D(&texDesc, 0, _sceneTexture);
		DX_CREATE_SRV(_sceneTexture.Get(), 0, _sceneSRV);
		DX_CREATE_RTV(_sceneTexture.Get(), nullptr, _sceneRTV);
	}

	DX_CREATE_RTV(_backBufferTexture.Get(), nullptr, _renderTargetView);

	{
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = static_cast<uint32>(sceneWidth);
		texDesc.Height = static_cast<uint32>(sceneHeight);
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		DX_CREATE_TEXTURE2D(&texDesc, 0, _hdrTexture);
		DX_CREATE_SRV(_hdrTexture.Get(), 0, _hdrSRV);
		DX_CREATE_RTV(_hdrTexture.Get(), 0, _hdrRTV);

		for (int i = 0; i < _postProcesses.size(); i++)
		{
			_postProcesses[i]->SetHDR_SRV(_hdrSRV);
		}

		// Without toneMapping
		for (int i = 0; i < _postProcesses.size() - 1; i++)
		{
            ComPtr<ID3D11Texture2D> ppTex;
            ComPtr<ID3D11ShaderResourceView> ppSRV;
            ComPtr<ID3D11RenderTargetView> ppRTV;

			DX_CREATE_TEXTURE2D(&texDesc, 0, ppTex);
			DX_CREATE_SRV(ppTex.Get(), 0, ppSRV);
			DX_CREATE_RTV(ppTex.Get(), 0, ppRTV);

            _ppTextures.push_back(ppTex);
            _ppSRVs.push_back(ppSRV);
            _ppRTVs.push_back(ppRTV);

            unique_ptr<Texture> debugTexture = make_unique<Texture>();
			debugTexture->SetSRV(_ppSRVs[i]);
			debugTexture->SetSize({ sceneWidth, sceneHeight });
            _ppDebugTextures.push_back(RESOURCES->AllocateTempResource(std::move(debugTexture)));
		}
	}
}

void Graphics::CreateDSVAndShadowMap(bool createShadowMap)
{
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = static_cast<uint32>(GAME->GetGameDesc().sceneWidth);
		desc.Height = static_cast<uint32>(GAME->GetGameDesc().sceneHeight);
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		DX_CREATE_TEXTURE2D(&desc, nullptr, _depthStencilTexture);

		if (createShadowMap)
		{
			desc.Width = SHADOWMAP_SIZE;
			desc.Height = SHADOWMAP_SIZE;
			desc.ArraySize = NUM_SHADOW_CASCADES;
			desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
			desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			DX_CREATE_TEXTURE2D(&desc, nullptr, _shadowDSTexture);
		}
	}

	{
		D3D11_DEPTH_STENCIL_VIEW_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Flags = 0;
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;

		DX_CREATE_DSV(_depthStencilTexture.Get(), &desc, _depthStencilView);

		if (createShadowMap)
		{
			desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			for (int i = 0; i < NUM_SHADOW_CASCADES; i++)
			{
				desc.Texture2DArray.FirstArraySlice = i;
				desc.Texture2DArray.ArraySize = 1;
				DX_CREATE_DSV(_shadowDSTexture.Get(), &desc, _shadowDSV[i]);
			}
		}
	}

	if (createShadowMap)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;

		for (int i = 0; i < NUM_SHADOW_CASCADES; i++)
		{
			ComPtr<ID3D11ShaderResourceView> srv;
			srvDesc.Texture2DArray.FirstArraySlice = i;
			srvDesc.Texture2DArray.ArraySize = 1;
			DX_CREATE_SRV(_shadowDSTexture.Get(), &srvDesc, srv);

			RESOURCES->GetAssetSlot().Remove(_shadowMap[i]);
			_shadowMap[i] = RESOURCES->AllocateTempResource(make_unique<Texture>());
			_shadowMap[i].Resolve()->SetSRV(srv);
            _shadowMap[i].Resolve()->SetSize({ SHADOWMAP_SIZE, SHADOWMAP_SIZE });
		}

		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = NUM_SHADOW_CASCADES;
		DX_CREATE_SRV(_shadowDSTexture.Get(), &srvDesc, _shadowArraySRV);
	}
}

void Graphics::SetViewport(float width, float height, float x, float y, float minDepth, float maxDepth)
{
	_vp.Set(width, height, x, y, minDepth, maxDepth);
	_shadowVP.Set(SHADOWMAP_SIZE, SHADOWMAP_SIZE, x, y, minDepth, maxDepth);
}
