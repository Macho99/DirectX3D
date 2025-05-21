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

	void SetMesh(shared_ptr<Mesh> mesh) { _mesh = mesh; }

	void RenderInstancing(shared_ptr<class InstancingBuffer>& buffer, bool isShadowTech);
	InstanceID GetInstanceID();

private:
	shared_ptr<Mesh> _mesh;
};

