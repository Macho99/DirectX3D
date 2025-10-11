#pragma once
#include "PostProcess.h"

class ToneMapping : public PostProcess
{
    using Super = PostProcess;

public:
    ToneMapping();
    void Render(ComPtr<ID3D11ShaderResourceView> srv, ComPtr<ID3D11RenderTargetView> rtv) override;

private:
    shared_ptr<Material> _toneMappingMat;
};

