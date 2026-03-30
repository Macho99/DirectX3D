#pragma once
#include "Renderer.h"

class Model;
class Shader;
class Material;

class ModelRenderer : public Renderer
{
	using Super = Renderer;
	DECLARE_COMPONENT(ModelRenderer)
public:
	ModelRenderer();
	virtual ~ModelRenderer();

    void SetShader(ResourceRef<Shader> shader);
	void SetModel(ResourceRef<Model> model);

	void RenderInstancing(shared_ptr<class InstancingBuffer>& buffer, RenderTech renderTech);
	InstanceID GetInstanceID();

	void SetMaterial(ResourceRef<Material> material) override;

    virtual bool OnGUI() override;
	virtual bool TryInitialize() override;
    virtual void SubmitTriangles(const Bounds& explicitBounds, vector<InputTri>& tris) override;

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(
			CEREAL_NVP(_shader), 
			CEREAL_NVP(_model)
		);
    }

private:
	ResourceRef<Shader>	_shader;
	ResourceRef<Model>	_model;

    bool _initialized = false;
};

