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

    void SetShader(ResourceRef<Shader> shader);
	void SetModel(ResourceRef<Model> model);

	void RenderInstancing(shared_ptr<class InstancingBuffer>& buffer, RenderTech renderTech);
	InstanceID GetInstanceID();

	void SetMaterial(ResourceRef<Material> material) override;

    virtual void LateUpdate() override;
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
			CEREAL_NVP(_shader), 
			CEREAL_NVP(_model),
            CEREAL_NVP(_originInstDatas)
		);
    }

    void AddInstancingData(const Matrix& mat);
    const vector<InstancingData>& GetInstancingDatas() const { return _instDatas; }
    bool HasInstancingData() const { return _originInstDatas.size() > 0; }

private:
    void FoliageSetup();

private:
	ResourceRef<Shader>	_shader;
	ResourceRef<Model>	_model;

    vector<InstancingData> _originInstDatas;
    vector<InstancingData> _instDatas;
    Matrix _lastWorldMatrix;

    bool _initialized = false;
};

