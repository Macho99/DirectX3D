#include "pch.h"
#include "Bloom.h"
#include "MeshRenderer.h"
#include "Material.h"

Bloom::Bloom()
{
	_brightFilterMat = make_shared<Material>();
	shared_ptr<Texture> texture = make_shared<Texture>();
	_brightFilterMat->SetDiffuseMap(texture);
	_brightFilterMat->SetShader(make_shared<Shader>(L"Bloom.fx"));
	_brightFilterMat->GetShader()->SetTechNum(RenderTech::Draw, 0);
}

void Bloom::Render(ComPtr<ID3D11ShaderResourceView> srv, ComPtr<ID3D11RenderTargetView> rtv)
{
    Super::Render(srv, rtv);
    _brightFilterMat->GetDiffuseMap()->SetSRV(srv.Get());
    DrawQuad(_brightFilterMat.get());
}
