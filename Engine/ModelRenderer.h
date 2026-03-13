#pragma once
#include "Renderer.h"

class Model;
class Shader;
class Material;

class ModelRenderer : public Renderer
{
	using Super = Renderer;

public:
	static constexpr ComponentType StaticType = ComponentType::ModelRenderer;
	ModelRenderer();
	virtual ~ModelRenderer();

    void SetShader(ResourceRef<Shader> shader);
	void SetModel(ResourceRef<Model> model);

	void RenderInstancing(shared_ptr<class InstancingBuffer>& buffer, RenderTech renderTech);
	InstanceID GetInstanceID();

	void SetMaterial(ResourceRef<Material> material) override;

    virtual bool OnGUI() override;
	virtual bool TryInitialize() override;

private:
	ResourceRef<Shader>	_shader;
	ResourceRef<Model>	_model;

    bool _initialized = false;
};

