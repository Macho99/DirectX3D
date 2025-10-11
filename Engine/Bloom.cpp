#include "pch.h"
#include "Bloom.h"
#include "MeshRenderer.h"
#include "Material.h"

Bloom::Bloom()
{
    {
        _brightFilterMat = make_shared<Material>();
        shared_ptr<Texture> texture = make_shared<Texture>();
        _brightFilterMat->SetDiffuseMap(texture);
        _brightFilterMat->SetShader(make_shared<Shader>(L"Bloom.fx"));
        _brightFilterMat->GetShader()->SetTechNum(RenderTech::Draw, 0);
    }

    {
        _downSampleMat = make_shared<Material>();
        shared_ptr<Texture> texture = make_shared<Texture>();
        _downSampleMat->SetDiffuseMap(texture);
        _downSampleMat->SetShader(make_shared<Shader>(L"PrintTexture.fx"));
        _downSampleMat->GetShader()->SetTechNum(RenderTech::Draw, 0);
    }

    {
        _combineMat = make_shared<Material>();
        shared_ptr<Texture> texture = make_shared<Texture>();
        _combineMat->SetDiffuseMap(texture);
        shared_ptr<Texture> texture2 = make_shared<Texture>();
        _combineMat->SetSpecularMap(texture2);
        _combineMat->SetShader(make_shared<Shader>(L"BlurCombine.fx"));
        _combineMat->GetShader()->SetTechNum(RenderTech::Draw, 0);
    }

    OnSize(GAME->GetGameDesc().width, GAME->GetGameDesc().height);
}

void Bloom::Render(ComPtr<ID3D11ShaderResourceView> srv, ComPtr<ID3D11RenderTargetView> rtv)
{
    DownSample(0, srv);
    
    _brightFilterMat->GetDiffuseMap()->SetSRV(_downSampleSRVs[0].Get());
    DC->ClearRenderTargetView(_brightFilterRTV.Get(), reinterpret_cast<const float*>(&Colors::Black));
    DC->OMSetRenderTargets(1, _brightFilterRTV.GetAddressOf(), 0);
    DrawQuad(_brightFilterMat.get());
    
    DownSample(1, _brightFilterSRV);
    for (int i = 2; i < _sampleSize.size(); i++)
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
    
    GRAPHICS->GetViewport().RSSetViewport();
    Super::Render(srv, rtv);
    _combineMat->GetDiffuseMap()->SetSRV(_upSampleSRVs.back().Get());
    _combineMat->GetSpecularMap()->SetSRV(srv);
    DrawQuad(_combineMat.get());
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
		HR(DEVICE->CreateTexture2D(&texDesc, 0, _downSampleTextures[i].GetAddressOf()));
		HR(DEVICE->CreateShaderResourceView(_downSampleTextures[i].Get(), 0, _downSampleSRVs[i].GetAddressOf()));
		HR(DEVICE->CreateRenderTargetView(_downSampleTextures[i].Get(), 0, _downSampleRTVs[i].GetAddressOf()));

        int upSampleIdx = _sampleSize.size() - 1 - i;
        _upSampleDescs[upSampleIdx] = texDesc;
		HR(DEVICE->CreateTexture2D(&texDesc, 0, _upSampleTextures[upSampleIdx].GetAddressOf()));
		HR(DEVICE->CreateShaderResourceView(_upSampleTextures[upSampleIdx].Get(), 0, _upSampleSRVs[upSampleIdx].GetAddressOf()));
		HR(DEVICE->CreateRenderTargetView(_upSampleTextures[upSampleIdx].Get(), 0, _upSampleRTVs[upSampleIdx].GetAddressOf()));
        _blurs[upSampleIdx].OnSize(curWidth, curHeight);
    }

    {
        HR(DEVICE->CreateTexture2D(&_downSampleDescs[0], 0, _brightFilterTexture.GetAddressOf()));
        HR(DEVICE->CreateShaderResourceView(_brightFilterTexture.Get(), 0, _brightFilterSRV.GetAddressOf()));
        HR(DEVICE->CreateRenderTargetView(_brightFilterTexture.Get(), 0, _brightFilterRTV.GetAddressOf()));
    }
}

void Bloom::SetDebugTextureSRV(shared_ptr<Texture> texture)
{
    texture->SetSRV(_upSampleSRVs[2].Get());
}

void Bloom::DownSample(int index, ComPtr<ID3D11ShaderResourceView> srv)
{
    _vp.Set(_downSampleDescs[index].Width, _downSampleDescs[index].Height);
    DC->ClearRenderTargetView(_downSampleRTVs[index].Get(), reinterpret_cast<const float*>(&Colors::Black));
    DC->OMSetRenderTargets(1, _downSampleRTVs[index].GetAddressOf(), 0);
    _vp.RSSetViewport();

    _downSampleMat->GetDiffuseMap()->SetSRV(srv);
    DrawQuad(_downSampleMat.get());
}

void Bloom::UpSample(int index, ComPtr<ID3D11ShaderResourceView> accumulateSrv)
{
    _vp.Set(_upSampleDescs[index].Width, _upSampleDescs[index].Height);
    DC->ClearRenderTargetView(_upSampleRTVs[index].Get(), reinterpret_cast<const float*>(&Colors::Black));
    DC->OMSetRenderTargets(1, _upSampleRTVs[index].GetAddressOf(), 0);
    _vp.RSSetViewport();

    _combineMat->GetDiffuseMap()->SetSRV(_upSampleSRVs[index - 1].Get());
    _combineMat->GetSpecularMap()->SetSRV(accumulateSrv);
    DrawQuad(_combineMat.get());
}

void Bloom::ProcessBlur(int index)
{
    DC->CopyResource(_blurs[index].GetTexture2D(), _upSampleTextures[index].Get());
    _blurs[index].ProcessBlur(1);
    DC->CopyResource(_upSampleTextures[index].Get(), _blurs[index].GetTexture2D());
}
