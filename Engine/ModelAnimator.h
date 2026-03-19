#pragma once
#include "Renderer.h"

class Model;

struct AnimTransform
{
	using TransformArrayType = array<Matrix, MAX_MODEL_TRANSFORMS>;
	array<TransformArrayType, MAX_MODEL_KEYFRAMES> transforms;
};

class ModelAnimator : public Renderer
{
    using Super = Renderer;
    DECLARE_COMPONENT(ModelAnimator)
public:
	ModelAnimator();
	~ModelAnimator();

	virtual void Update() override;

	void UpdateTweenData();

    void SetShader(ResourceRef<Shader> shader);
	void SetModel(ResourceRef<Model> model);

	void RenderInstancing(shared_ptr<class InstancingBuffer>& buffer, RenderTech renderTech);
	InstanceID GetInstanceID();
	TweenDesc& GetTweenDesc() { return _tweenDesc; }

    virtual bool OnGUI() override;
    virtual bool TryInitialize() override;

    template<typename Archive>
	void serialize(Archive& ar)
	{
		Super::serialize(ar);
		ar(
			CEREAL_NVP(_shader),
			CEREAL_NVP(_model),
			CEREAL_NVP(_keyframeDesc)
			);
	}

private:
	void CreateTexture();
	void CreateAnimationTransform(uint32 index);

private:
	vector<AnimTransform> _animTransforms;
	ComPtr<ID3D11Texture2D> _texture;
	ComPtr<ID3D11ShaderResourceView> _srv;

private:
	KeyframeDesc _keyframeDesc;
	TweenDesc _tweenDesc;

private:
	ResourceRef<Shader> _shader;
    ResourceRef<Model> _model;

    bool _initialized = false;
};

