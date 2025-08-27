#pragma once
#include "PostProcess.h"

class Bloom : public PostProcess
{
    using Super = PostProcess;

public:
    Bloom();
    void Render(ComPtr<ID3D11ShaderResourceView> srv, ComPtr<ID3D11RenderTargetView> rtv) override;

private:
    shared_ptr<Material> _material;
};

