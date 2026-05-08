#pragma once
#include "Renderer.h"
class SsrRenderer : public Renderer
{
    using Super = Renderer;
    DECLARE_COMPONENT(SsrRenderer, Renderer)

public:
    SsrRenderer();
    ~SsrRenderer();
    void InnerRender(RenderTech renderTech) override;
    virtual bool OnGUI() override;
    virtual void SubmitTriangles(const Bounds& explicitBounds, vector<InputTri>& tris) override;

    void SetMesh(ResourceRef<Mesh> mesh) { _mesh = mesh; }
    ResourceRef<Mesh> GetMesh() { return _mesh; }

private:
    ResourceRef<Mesh> _mesh;
};

