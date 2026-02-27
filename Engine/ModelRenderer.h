#pragma once
#include "Renderer.h"

class Model;
class Shader;
class Material;

class ModelRenderer : public Renderer
{
	using Super = Renderer;

public:
	ModelRenderer(ResourceRef<Shader> shader);
	virtual ~ModelRenderer();

	void SetModel(ResourceRef<Model> model);
	void SetPass(uint8 pass) { _pass = pass; }

	void RenderInstancing(shared_ptr<class InstancingBuffer>& buffer, RenderTech renderTech);
	InstanceID GetInstanceID();

	void SetMaterial(ResourceRef<Material> material) override;

private:
	ResourceRef<Shader>	_shader;
	uint8				_pass = 0;
	ResourceRef<Model>	_model;
};

