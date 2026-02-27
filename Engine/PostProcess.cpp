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
    Mesh* mesh = _mesh.Resolve();
    if (mesh == nullptr)
    {
        _mesh = RESOURCES->GetQuadMesh();
        mesh = _mesh.Resolve();
    }

    DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    material->Update();
    mesh->GetVertexBuffer()->PushData();
    mesh->GetIndexBuffer()->PushData();
    material->GetShader()->DrawIndexed(RenderTech::Draw, 0, mesh->GetIndexBuffer()->GetCount());
}
