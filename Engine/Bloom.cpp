#include "pch.h"
#include "Bloom.h"
#include "MeshRenderer.h"
#include "Material.h"

Bloom::Bloom()
{
	_material = make_shared<Material>();
	shared_ptr<Texture> texture = make_shared<Texture>();
	_material->SetDiffuseMap(texture);
	_material->SetShader(make_shared<Shader>(L"Bloom.fx"));
	_material->GetShader()->SetTechNum(RenderTech::Draw, 0);
}

void Bloom::Render(ComPtr<ID3D11ShaderResourceView> srv, ComPtr<ID3D11RenderTargetView> rtv)
{
    Super::Render(srv, rtv);
    _material->GetDiffuseMap()->SetSRV(srv.Get());
    DrawQuad(_material.get());
}
