#include "pch.h"
#include "Bloom.h"
#include "MeshRenderer.h"
#include "Material.h"
#include "FileUtils.h"

Bloom::Bloom()
{
    {
        unique_ptr<Material> brightFilterMat = make_unique<Material>();
        unique_ptr<Texture> texture = make_unique<Texture>();
        ResourceRef<Texture> textureRef = RESOURCES->AllocateTempResource<Texture>(std::move(texture));
        brightFilterMat->SetDiffuseMap(textureRef);
        brightFilterMat->SetShader(RESOURCES->GetResourceRefByPath<Shader>(L"\\Shaders\\Bloom.fx"));
        brightFilterMat->GetShader()->SetTechNum(RenderTech::Draw, 0);
        _brightFilterMat = RESOURCES->AllocateTempResource<Material>(std::move(brightFilterMat));
    }

    {
        unique_ptr<Material> downSampleMat = make_unique<Material>();
        unique_ptr<Texture> texture = make_unique<Texture>();
        ResourceRef<Texture> textureRef = RESOURCES->AllocateTempResource<Texture>(std::move(texture));
        downSampleMat->SetDiffuseMap(textureRef);
        downSampleMat->SetShader(RESOURCES->GetResourceRefByPath<Shader>(L"\\Shaders\\DownSample.fx"));
        downSampleMat->GetShader()->SetTechNum(RenderTech::Draw, 0);
        _downSampleMat = RESOURCES->AllocateTempResource<Material>(std::move(downSampleMat));
    }

    {
        unique_ptr<Material> combineMat = make_unique<Material>();
        unique_ptr<Texture> texture = make_unique<Texture>();
        ResourceRef<Texture> textureRef = RESOURCES->AllocateTempResource<Texture>(std::move(texture));
        combineMat->SetDiffuseMap(textureRef);

        unique_ptr<Texture> texture2 = make_unique<Texture>();
        ResourceRef<Texture> textureRef2 = RESOURCES->AllocateTempResource<Texture>(std::move(texture));
        combineMat->SetSpecularMap(textureRef2);

        combineMat->SetShader(RESOURCES->GetResourceRefByPath<Shader>(L"\\Shaders\\BlurCombine.fx"));
        combineMat->GetShader()->SetTechNum(RenderTech::Draw, 0);
        _combineMat = RESOURCES->AllocateTempResource<Material>(std::move(combineMat));
    }

    OnSize(GAME->GetGameDesc().sceneWidth, GAME->GetGameDesc().sceneHeight);
}

Bloom::~Bloom()
{
}

void Bloom::SetHDR_SRV(ComPtr<ID3D11ShaderResourceView> srv)
{
    _hdrSRV = srv;
    _brightFilterMat.Resolve()->GetDiffuseMap().Resolve()->SetSRV(srv);
}

void Bloom::Render(ComPtr<ID3D11RenderTargetView> rtv)
{
    DC->ClearRenderTargetView(_brightFilterRTV.Get(), reinterpret_cast<const float*>(&Colors::Black));
    DC->OMSetRenderTargets(1, _brightFilterRTV.GetAddressOf(), 0);
    DrawQuad(_brightFilterMat.Resolve());

    DownSample(0, _brightFilterSRV);
    
    for (int i = 1; i < _sampleSize.size(); i++)
    {
        DownSample(i, _downSampleSRVs[i - 1]);
    }

    DC->CopyResource(_upSampleTextures[0].Get(), _downSampleTextures.back().Get());
    ProcessBlur(0);
    for (int i = 1; i < _sampleSize.size() - 1; i++)
    {
        UpSample(i, _downSampleSRVs[_sampleSize.size() - i - 1]);
        ProcessBlur(i);
    }
    UpSample(_sampleSize.size() - 1, _brightFilterSRV);

    if (INPUT->GetButtonDown(KEY_TYPE::KEY_1) && INPUT->GetButton(KEY_TYPE::LSHIFT))
    {
        FileUtils::SaveTextureToFile(_brightFilterTexture.Get(), L"../DebugTextures/BrightFilterTex.png");
        for (int i = 0; i < _downSampleTextures.size(); i++)
            FileUtils::SaveTextureToFile(_downSampleTextures[i].Get(), (L"../DebugTextures/DownSampleTex" + to_wstring(i) + L".png").c_str());
        for (int i = 0; i < _upSampleTextures.size(); i++)
            FileUtils::SaveTextureToFile(_upSampleTextures[i].Get(), (L"../DebugTextures/UpSampleTex" + to_wstring(i + _downSampleTextures.size()) + L".png").c_str());
    }

    GRAPHICS->GetViewport().RSSetViewport();
    Super::Render(rtv);
    _combineMat.Resolve()->GetDiffuseMap().Resolve()->SetSRV(_upSampleSRVs.back());
    _combineMat.Resolve()->GetSpecularMap().Resolve()->SetSRV(_hdrSRV);
    DrawQuad(_combineMat.Resolve());
}

void Bloom::OnSize(UINT width, UINT height)
{
    Super::OnSize(width, height);

	_downSampleDescs.clear();
    _downSampleTextures.clear();
    _downSampleSRVs.clear();
    _downSampleRTVs.clear();

    _upSampleDescs.clear();
    _upSampleTextures.clear();
    _upSampleSRVs.clear();
    _upSampleRTVs.clear();

    _downSampleDescs.resize(_sampleSize.size());
    _downSampleTextures.resize(_sampleSize.size());
    _downSampleSRVs.resize(_sampleSize.size());
    _downSampleRTVs.resize(_sampleSize.size());

    _upSampleDescs.resize(_sampleSize.size());
    _upSampleTextures.resize(_sampleSize.size());
    _upSampleSRVs.resize(_sampleSize.size());
    _upSampleRTVs.resize(_sampleSize.size());

    UINT prevWidth = width;
    UINT prevHeight = height;
    
    for (int i = 0; i < _sampleSize.size(); i++)
    {
        UINT curWidth = max(1, prevWidth / _sampleSize[i]);
        UINT curHeight = max(1, prevHeight / _sampleSize[i]);

        prevWidth = curWidth;
        prevHeight = curHeight;

		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = static_cast<uint32>(curWidth);
		texDesc.Height = static_cast<uint32>(curHeight);
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

        _downSampleDescs[i] = texDesc;
        DX_CREATE_TEXTURE2D(&texDesc, 0, _downSampleTextures[i]);
        DX_CREATE_SRV(_downSampleTextures[i].Get(), 0, _downSampleSRVs[i]);
        DX_CREATE_RTV(_downSampleTextures[i].Get(), 0, _downSampleRTVs[i]);

        int upSampleIdx = _sampleSize.size() - 1 - i;
        _upSampleDescs[upSampleIdx] = texDesc;
        DX_CREATE_TEXTURE2D(&texDesc, 0, _upSampleTextures[upSampleIdx]);
        DX_CREATE_SRV(_upSampleTextures[upSampleIdx].Get(), 0, _upSampleSRVs[upSampleIdx]);
        DX_CREATE_RTV(_upSampleTextures[upSampleIdx].Get(), 0, _upSampleRTVs[upSampleIdx]);
        _blurs[upSampleIdx].OnSize(curWidth, curHeight);
    }

    {
        D3D11_TEXTURE2D_DESC desc = _downSampleDescs[0];
        desc.Width = width;
        desc.Height = height;

        DX_CREATE_TEXTURE2D(&desc, 0, _brightFilterTexture);
        DX_CREATE_SRV(_brightFilterTexture.Get(), 0, _brightFilterSRV);
        DX_CREATE_RTV(_brightFilterTexture.Get(), 0, _brightFilterRTV);
    }
}

void Bloom::SetDebugTextureSRV(ResourceRef<Texture> texture)
{
    texture.Resolve()->SetSRV(_upSampleSRVs[2].Get());
}

void Bloom::DownSample(int index, ComPtr<ID3D11ShaderResourceView> srv)
{
    int targetWidth = _downSampleDescs[index].Width;
    int targetHeight = _downSampleDescs[index].Height;

    _vp.Set(targetWidth, targetHeight);
    DC->ClearRenderTargetView(_downSampleRTVs[index].Get(), reinterpret_cast<const float*>(&Colors::Black));
    DC->OMSetRenderTargets(1, _downSampleRTVs[index].GetAddressOf(), 0);
    _vp.RSSetViewport();

    _downSampleMat.Resolve()->GetShader()->PushBlurData({ 1.0f / targetWidth , 1.0f / targetHeight});
    _downSampleMat.Resolve()->GetDiffuseMap().Resolve()->SetSRV(srv);
    DrawQuad(_downSampleMat.Resolve());
}

void Bloom::UpSample(int index, ComPtr<ID3D11ShaderResourceView> accumulateSrv)
{
    _vp.Set(_upSampleDescs[index].Width, _upSampleDescs[index].Height);
    DC->ClearRenderTargetView(_upSampleRTVs[index].Get(), reinterpret_cast<const float*>(&Colors::Black));
    DC->OMSetRenderTargets(1, _upSampleRTVs[index].GetAddressOf(), 0);
    _vp.RSSetViewport();

    _combineMat.Resolve()->GetDiffuseMap().Resolve()->SetSRV(_upSampleSRVs[index - 1].Get());
    _combineMat.Resolve()->GetSpecularMap().Resolve()->SetSRV(accumulateSrv);
    DrawQuad(_combineMat.Resolve());
}

void Bloom::ProcessBlur(int index)
{
    DC->CopyResource(_blurs[index].GetTexture2D(), _upSampleTextures[index].Get());
    _blurs[index].ProcessBlur(1);
    DC->CopyResource(_upSampleTextures[index].Get(), _blurs[index].GetTexture2D());
}
