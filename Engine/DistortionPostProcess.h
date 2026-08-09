#pragma once
#include "PostProcess.h"

class DistortionPostProcess : public PostProcess
{
    using Super = PostProcess;

public:
    DistortionPostProcess();
    void SetHDR_SRV(ComPtr<ID3D11ShaderResourceView> srv) override;
    void Render(ComPtr<ID3D11RenderTargetView> rtv) override;
    virtual void SetDebugTextureSRV(ResourceRef<Texture> texture) override;

private:
    ResourceRef<Material> _material;
};
