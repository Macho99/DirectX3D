#pragma once
#include "Renderer.h"

class Mesh;
class Shader;
class Material;

class MeshRenderer : public Renderer
{
	using Super = Renderer;
public:
	MeshRenderer();
	virtual ~MeshRenderer();

	void SetMesh(ResourceRef<Mesh> mesh) { _mesh = mesh; }
	bool Render(RenderTech renderTech) override;
	void RenderInstancing(shared_ptr<class InstancingBuffer>& buffer, RenderTech renderTech);
	InstanceID GetInstanceID();

private:
	ResourceRef<Mesh> _mesh;
};

