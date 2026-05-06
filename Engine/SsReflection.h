#pragma once
#include "PostProcess.h"
class SsReflection : public PostProcess
{
    using Super = PostProcess;
public:
    SsReflection();
    void SetHDR_SRV(ComPtr<ID3D11ShaderResourceView> srv) override;
    void Render(ComPtr<ID3D11RenderTargetView> rtv) override;

    virtual bool NeedCopiedHDRTexture() const override { return true; }

private:
    ComPtr<ID3D11ShaderResourceView> _hdrSRV;
    ComponentRef<class SsrRenderer> _ssrRenderer;
};

