#pragma once
#include "PostProcess.h"
#include "Blur.h"

class Bloom : public PostProcess
{
    using Super = PostProcess;

public:
    Bloom();
    ~Bloom();
    void Render(ComPtr<ID3D11ShaderResourceView> srv, ComPtr<ID3D11RenderTargetView> rtv) override;
    void OnSize(UINT width, UINT height) override;
    void SetDebugTextureSRV(shared_ptr<Texture> texture) override;

private:
    void DownSample(int index, ComPtr<ID3D11ShaderResourceView> srv);
    void UpSample(int index, ComPtr<ID3D11ShaderResourceView> accumulateSrv);
    void ProcessBlur(int index);

private:
    shared_ptr<Material> _brightFilterMat;
    shared_ptr<Material> _gaussianBlurMat;
    shared_ptr<Material> _downSampleMat;
    shared_ptr<Material> _combineMat;

    array<int, 3> _sampleSize = {4, 6, 6};

    Viewport _vp;

    ComPtr<ID3D11Texture2D> _brightFilterTexture;
    ComPtr<ID3D11ShaderResourceView> _brightFilterSRV;
    ComPtr<ID3D11RenderTargetView> _brightFilterRTV;

    vector<D3D11_TEXTURE2D_DESC> _downSampleDescs;
    vector<ComPtr<ID3D11Texture2D>> _downSampleTextures;
    vector<ComPtr<ID3D11ShaderResourceView>> _downSampleSRVs;
    vector<ComPtr<ID3D11RenderTargetView>> _downSampleRTVs;

    array<Blur, 3> _blurs;
    vector<D3D11_TEXTURE2D_DESC> _upSampleDescs;
    vector<ComPtr<ID3D11Texture2D>> _upSampleTextures;
    vector<ComPtr<ID3D11ShaderResourceView>> _upSampleSRVs;
    vector<ComPtr<ID3D11RenderTargetView>> _upSampleRTVs;
};

