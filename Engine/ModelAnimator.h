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

public:
	ModelAnimator(ResourceRef<Shader> shader);
	~ModelAnimator();

	virtual void Update() override;

	void UpdateTweenData();

	void SetModel(ResourceRef<Model> model);

	void RenderInstancing(shared_ptr<class InstancingBuffer>& buffer, RenderTech renderTech);
	InstanceID GetInstanceID();
	TweenDesc& GetTweenDesc() { return _tweenDesc; }

    virtual bool OnGUI() override;

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
};

