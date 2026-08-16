#pragma once
#include "InstancingRenderer.h"

class Model;
class Shader;
class Material;

class ModelRenderer : public InstancingRenderer
{
	using Super = InstancingRenderer;
	DECLARE_COMPONENT(ModelRenderer)
public:
	ModelRenderer();
	virtual ~ModelRenderer();

	bool SetModel(ResourceRef<Model> model);
	ResourceRef<Model> GetModel() const { return _model; }
	bool IsInFrustum(const Vec4 frustumPlanes[6]);

    virtual void RenderInstancing(class InstancingBuffer& buffer, RenderTech renderTech) override;

    virtual bool OnGUI() override;
    virtual void OnMenu() override;
	virtual bool TryInitialize() override;
    virtual void SubmitTriangles(const Bounds& explicitBounds, vector<InputTri>& tris) override;
    virtual AssetId GetMeshId() const override { return _model.GetAssetId(); }

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(
			CEREAL_NVP(_model)
		);
    }

private:
    void FoliageSetup();
	void UpdateLocalBounds();

private:
	ResourceRef<Model>	_model;
	BoundingBox _localBounds;
	BoundingBox _worldBounds;
	bool _hasLocalBounds = false;
	bool _boundsInitialized = false;

    bool _initialized = false;
};

