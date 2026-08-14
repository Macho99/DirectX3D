#pragma once
#include "InstancingRenderer.h"
//#include "skinned_mesh.h"

class Model;

// 블렌드 스페이스의 한 샘플 지점.
// position은 XY 좌표, animIndex는 Model에 등록된 애니메이션 인덱스를 가리킨다.
// speed는 해당 애니메이션의 재생 속도 배율이다.
struct BlendSpacePoint
{
	Vec2 position = Vec2::Zero;
	int32 animIndex = -1;
	float speed = 1.f;

	template<typename Archive>
	void serialize(Archive& ar)
	{
		ar(
			cereal::make_nvp("x", position.x),
			cereal::make_nvp("y", position.y),
			CEREAL_NVP(animIndex),
			CEREAL_NVP(speed));
	}
};

struct AnimTransform
{
	using TransformArrayType = array<Matrix, MAX_MODEL_TRANSFORMS>;
	array<TransformArrayType, MAX_MODEL_KEYFRAMES> transforms;
    array<Matrix, MAX_MODEL_KEYFRAMES> rootTransforms;
};

class ModelAnimator : public InstancingRenderer
{
    using Super = InstancingRenderer;
    DECLARE_COMPONENT(ModelAnimator)
public:
	ModelAnimator();
	~ModelAnimator();

    virtual void Awake() override;
	virtual void Update() override;

	void UpdateRootMotion(const TweenDesc prevTweenDesc);
	void UpdateTweenData();

	void SetModel(ResourceRef<Model> model);

	virtual void RenderInstancing(class InstancingBuffer& buffer, RenderTech renderTech) override;
	TweenDesc& GetTweenDesc() { return _tweenDesc; }

	// 외부 게임 로직에서 미리보기 입력 좌표와 샘플 지점을 관리할 때 사용하는 API.
	void SetBlendSpaceInput(const Vec2& input) { _blendSpaceInput = input; }
	const Vec2& GetBlendSpaceInput() const { return _blendSpaceInput; }
	int AddBlendSpacePoint(const Vec2& position, int32 animIndex, float speed = 1.f);
	bool RemoveBlendSpacePoint(int index);
	void ClearBlendSpace();
	const vector<BlendSpacePoint>& GetBlendSpacePoints() const { return _blendSpacePoints; }

	void PlayAnimation(uint32 animIndex);
    void PlayAnimation(string animName);
	bool TryGetModelSocketWorldMatrix(const string& socketName, OUT Matrix& worldMatrix);

    virtual bool OnGUI() override;
    virtual bool TryInitialize() override;
	virtual void OnInspectorFocusLost() override;
    virtual AssetId GetMeshId() const override { return _model.GetAssetId(); }

    template<typename Archive>
	void serialize(Archive& ar)
	{
		Super::serialize(ar);
		ar(
			CEREAL_NVP(_model),
			// 블렌드 스페이스 편집 결과를 컴포넌트와 함께 저장한다.
            CEREAL_NVP(_blendSpaceMin),
            CEREAL_NVP(_blendSpaceMax),
			CEREAL_NVP(_blendSpacePoints)
			);

		if constexpr (Archive::is_loading::value)
		{
			_blendSpaceTriangulationDirty = true;
		}
	}

private:
	struct BlendSpaceSample
	{
		array<int, 3> pointIndices = { -1, -1, -1 };
		array<float, 3> weights = { 0.0f, 0.0f, 0.0f };
		Vec2 sampledPosition = Vec2::Zero;
	};

	void CreateTexture();
	void CreateAnimationTransform(uint32 index);
	// 최초 사용 또는 점/애니메이션 변경 시에만 Delaunay 삼각망 캐시를 갱신한다.
	void UpdateBlendSpaceTriangulation();
	// 일반 애니메이션 한 개의 프레임 상태를 갱신한다.
	void UpdateRegularKeyframe(KeyframeDesc& keyframe);
	// 최대 3개 블렌드 애니메이션의 위상을 동기화하되 원래 사이클 속도는 가중 보간한다.
	void UpdateBlendSpaceKeyframe(KeyframeDesc& keyframe, const BlendSpaceSample& sample);
	// Delaunay 삼각망에서 입력 좌표가 속하거나 가장 가까운 삼각형을 찾는다.
	BlendSpaceSample EvaluateBlendSpace();
	// 점 편집과 삼각망/가중치 미리보기를 그린다.
	bool DrawBlendSpaceEditor();
	bool DrawDebugWindow();
	void DrawDebugMatrix(const char* label, const Matrix& matrix);
    float GetNormalizedTime(const AnimationFrameDesc& animFrame);
    float GetLeftTime(const AnimationFrameDesc& animFrame);

private:
	vector<AnimTransform> _animTransforms;
	ComPtr<ID3D11Texture2D> _texture;
	ComPtr<ID3D11ShaderResourceView> _srv;

private:
	TweenDesc _tweenDesc;
	int32 _nextAnimIndex = 0;

	// 블렌드 스페이스 편집 상태.
	Vec2 _blendSpaceInput = Vec2::Zero;
	Vec2 _blendSpaceMin = Vec2(-2.0f, -2.0f);
	Vec2 _blendSpaceMax = Vec2(2.0f, 2.0f);
	vector<BlendSpacePoint> _blendSpacePoints;
	int _selectedBlendSpacePoint = -1;

	// 매 프레임 삼각분할하지 않도록 인덱스 결과를 캐시한다.
	vector<array<int, 3>> _blendSpaceTriangles;
	bool _blendSpaceTriangulationDirty = true;
	int _blendSpaceCachedAnimationCount = -1;

private:
    ResourceRef<Model> _model;

    bool _initialized = false;

private:
	//SkinnedMesh _skinnedMesh;

private:
	bool _showAnimationDebug = false;
	int _debugAnimationIndex = 0;
	int _debugFrameIndex = 0;
    int _lastDebugFrameIndex = -1;
	int _debugBoneIndex = 0;
	int _debugSocketIndex = -1;
};

