#pragma once
#include "Renderer.h"

class Model;
class Shader;
class Material;

class ModelRenderer : public Renderer
{
	using Super = Renderer;

public:
	ModelRenderer(shared_ptr<Shader> shader);
	virtual ~ModelRenderer();

	void SetModel(shared_ptr<Model> model);
	void SetPass(uint8 pass) { _pass = pass; }

	void RenderInstancing(shared_ptr<class InstancingBuffer>& buffer);
	InstanceID GetInstanceID();

	void SetMaterial(shared_ptr<Material> material) override;

private:
	shared_ptr<Shader>	_shader;
	uint8				_pass = 0;
	shared_ptr<Model>	_model;
};

