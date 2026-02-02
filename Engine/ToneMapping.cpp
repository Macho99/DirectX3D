#include "pch.h"
#include "ToneMapping.h"
#include "MeshRenderer.h"
#include "Material.h"

ToneMapping::ToneMapping()
{
    _toneMappingMat = make_shared<Material>();
    shared_ptr<Texture> texture = make_shared<Texture>();
    _toneMappingMat->SetDiffuseMap(texture);
    _toneMappingMat->SetShader(make_shared<Shader>(L"ToneMapping.fx"));
    _toneMappingMat->GetShader()->SetTechNum(RenderTech::Draw, 0);
}

void ToneMapping::SetHDR_SRV(ComPtr<ID3D11ShaderResourceView> srv)
{
    _toneMappingMat->GetDiffuseMap()->SetSRV(srv);
}

void ToneMapping::Render(ComPtr<ID3D11RenderTargetView> rtv)
{
    Super::Render(rtv);
    DrawQuad(_toneMappingMat.get());
}
