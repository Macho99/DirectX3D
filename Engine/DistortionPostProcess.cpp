#include "pch.h"
#include "DistortionPostProcess.h"
#include "Material.h"

DistortionPostProcess::DistortionPostProcess()
{
    unique_ptr<Material> material = make_unique<Material>();
    material->SetDiffuseMap(RESOURCES->AllocateTempResource(make_unique<Texture>()));
    material->SetSpecularMap(GRAPHICS->GetDistortionMap());
    material->SetShader(RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\Distortion.fx"));
    material->GetShader()->SetTechNum(RenderTech::Draw, 0);
    _material = RESOURCES->AllocateTempResource(std::move(material));
}

void DistortionPostProcess::SetHDR_SRV(ComPtr<ID3D11ShaderResourceView> srv)
{
    _material.Resolve()->GetDiffuseMap().Resolve()->SetSRV(srv);
}

void DistortionPostProcess::Render(ComPtr<ID3D11RenderTargetView> rtv)
{
    Super::Render(rtv);
    DrawQuad(_material.Resolve());
}

void DistortionPostProcess::SetDebugTextureSRV(ResourceRef<Texture> texture)
{
    Texture* distortionMap = GRAPHICS->GetDistortionMap().Resolve();
    texture.Resolve()->SetSRV(distortionMap->GetComPtr());
}
