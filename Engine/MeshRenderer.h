#pragma once
#include "InstancingRenderer.h"

class Mesh;
class Shader;
class Material;

class MeshRenderer : public InstancingRenderer
{
	using Super = InstancingRenderer;
    DECLARE_COMPONENT(MeshRenderer)
public:
	MeshRenderer();
	virtual ~MeshRenderer();

	void SetMesh(ResourceRef<Mesh> mesh) { _mesh = mesh; }
    ResourceRef<Mesh> GetMesh() { return _mesh; }
	void InnerRender(RenderTech renderTech) override;
	void RenderInstancing(shared_ptr<class InstancingBuffer>& buffer, RenderTech renderTech);
    InstanceID GetInstanceID();
    virtual bool OnGUI() override;
    virtual void SubmitTriangles(const Bounds& explicitBounds, vector<InputTri>& tris) override;

    virtual AssetId GetMeshId() const override { return _mesh.GetAssetId(); }

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(_mesh);
    }

private:
	ResourceRef<Mesh> _mesh;
    int _debugTriangleCount = -1;
};

