#pragma once
#include "Renderer.h"

class Mesh;
class Shader;
class Material;

class MeshRenderer : public Renderer
{
	using Super = Renderer;
    DECLARE_COMPONENT(MeshRenderer)
public:
	MeshRenderer();
	virtual ~MeshRenderer();

	void SetMesh(ResourceRef<Mesh> mesh) { _mesh = mesh; }
	bool Render(RenderTech renderTech) override;
	void RenderInstancing(shared_ptr<class InstancingBuffer>& buffer, RenderTech renderTech);
    InstanceID GetInstanceID();
    virtual bool OnGUI() override;
    virtual void SubmitTriangles(const Bounds& explicitBounds, vector<InputTri>& tris) override;

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(_mesh);
    }

private:
	ResourceRef<Mesh> _mesh;
};

