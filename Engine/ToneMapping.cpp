#include "pch.h"
#include "ToneMapping.h"
#include "MeshRenderer.h"
#include "Material.h"

ToneMapping::ToneMapping()
{
    unique_ptr<Material> material = make_unique<Material>();
    shared_ptr<Texture> texture = make_shared<Texture>();
    material->SetDiffuseMap(RESOURCES->AllocateTempResource(make_unique<Texture>()));
    material->SetShader(RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\ToneMapping.fx"));
    material->GetShader()->SetTechNum(RenderTech::Draw, 0);
    _toneMappingMat = RESOURCES->AllocateTempResource(std::move(material));
}

void ToneMapping::SetHDR_SRV(ComPtr<ID3D11ShaderResourceView> srv)
{
    _toneMappingMat.Resolve()->GetDiffuseMap().Resolve()->SetSRV(srv);
}

void ToneMapping::Render(ComPtr<ID3D11RenderTargetView> rtv)
{
    Super::Render(rtv);
    DrawQuad(_toneMappingMat.Resolve());
}
