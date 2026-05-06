#include "pch.h"
#include "SsReflection.h"
#include "SsrRenderer.h"

SsReflection::SsReflection()
{
}

void SsReflection::SetHDR_SRV(ComPtr<ID3D11ShaderResourceView> srv)
{
    _hdrSRV = srv;
}

void SsReflection::Render(ComPtr<ID3D11RenderTargetView> rtv)
{
    {
        //Super::Render(rtv);
        DC->OMSetRenderTargets(1, rtv.GetAddressOf(), 0);

    }

    SsrRenderer* ssrRenderer = _ssrRenderer.Resolve();
    if(ssrRenderer == nullptr)
        _ssrRenderer = CUR_SCENE->FindComponentRef<SsrRenderer>();
    ssrRenderer = _ssrRenderer.Resolve();
    if (ssrRenderer == nullptr)
        return;

    Material* mat = ssrRenderer->GetMaterial().Resolve();
    if (mat == nullptr)
        return;

    if (mat->GetDiffuseMap().Resolve() == nullptr)
    {
        mat->SetDiffuseMap(RESOURCES->AllocateTempResource<Texture>());
        mat->GetDiffuseMap().Resolve()->SetSRV(_hdrSRV);
        mat->SetNormalMap(GRAPHICS->GetNormalDepthMap());
    }

    ssrRenderer->Render(RenderTech::Draw);
}
