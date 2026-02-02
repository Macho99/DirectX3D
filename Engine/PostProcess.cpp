#include "pch.h"
#include "PostProcess.h"
#include "Material.h"

PostProcess::PostProcess()
{
}

void PostProcess::Render(ComPtr<ID3D11RenderTargetView> rtv)
{
    DC->ClearRenderTargetView(rtv.Get(), reinterpret_cast<const float*>(&Colors::Black));
    DC->OMSetRenderTargets(1, rtv.GetAddressOf(), 0);
}

void PostProcess::DrawQuad(Material* material)
{
    if (_mesh == nullptr)
    {
        _mesh = RESOURCES->Get<Mesh>(L"Quad");
    }

    DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    material->Update();
    _mesh->GetVertexBuffer()->PushData();
    _mesh->GetIndexBuffer()->PushData();
    material->GetShader()->DrawIndexed(RenderTech::Draw, 0, _mesh->GetIndexBuffer()->GetCount());
}
