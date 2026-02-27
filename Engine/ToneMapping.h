#pragma once
#include "PostProcess.h"

class ToneMapping : public PostProcess
{
    using Super = PostProcess;

public:
    ToneMapping();
    void SetHDR_SRV(ComPtr<ID3D11ShaderResourceView> srv) override;
    void Render(ComPtr<ID3D11RenderTargetView> rtv) override;

private:
    ResourceRef<Material> _toneMappingMat;
};

