#include "pch.h"
#include "ModelAnimator.h"
#include "Material.h"
#include "ModelMesh.h"
#include "Model.h"
#include "ModelAnimation.h"
#include "Camera.h"
#include "Light.h"
#include "OnGUIUtils.h"
#include "MathLibrary/MathUtils.h"
#include "MonoBehaviour.h"
#include "SceneView.h"
#include "../MathLibrary/Geometry2D.h"

#include <cfloat>

namespace
{
	// 편집/직렬화 과정에서 생긴 미세한 좌표 오차 때문에 일직선 샘플이
	// 아주 얇은 삼각형으로 취급되지 않도록 좌표 범위에 비례해 판정한다.
	bool AreBlendSpacePointsCollinear(
		const vector<Geometry2D::Point>& points,
		const vector<int>& indices,
		int& lineStart,
		int& lineEnd)
	{
		lineStart = indices.empty() ? -1 : indices.front();
		lineEnd = lineStart;
		float maxDistanceSquared = 0.0f;

		for (size_t i = 0; i < indices.size(); ++i)
		{
			for (size_t j = i + 1; j < indices.size(); ++j)
			{
				const float distanceSquared = Geometry2D::LengthSquared(
					points[indices[j]] - points[indices[i]]);
				if (distanceSquared > maxDistanceSquared)
				{
					maxDistanceSquared = distanceSquared;
					lineStart = indices[i];
					lineEnd = indices[j];
				}
			}
		}

		if (maxDistanceSquared <= Geometry2D::DefaultEpsilon * Geometry2D::DefaultEpsilon)
			return true;

		const float lineLength = sqrt(maxDistanceSquared);
		const float distanceTolerance = max(Geometry2D::DefaultEpsilon, lineLength * 1e-4f);
		for (int index : indices)
		{
			const float twiceArea = abs(Geometry2D::Cross(
				points[lineStart], points[lineEnd], points[index]));
			if (twiceArea > distanceTolerance * lineLength)
				return false;
		}

		return true;
	}
}

ModelAnimator::ModelAnimator()
	: Super(StaticType)
{
    _pass = 2;
}

ModelAnimator::~ModelAnimator()
{
}

void ModelAnimator::Awake()
{
    static int awakeCount = 0;
	if (awakeCount++ != 0)
		return;

	if (_blendSpacePoints.size() == 0)
	{
		BlendSpacePoint center = { Vec2(0.f, 0.f), 27 };
		_blendSpacePoints.push_back(center);
		{
			BlendSpacePoint left = { Vec2(-1.0f, 0.0f), 43 };
			_blendSpacePoints.push_back(left);
			BlendSpacePoint right = { Vec2(1.0f, 0.0f), 46 };
			_blendSpacePoints.push_back(right);
			BlendSpacePoint top = { Vec2(0.0f, 1.0f), 50 };
			_blendSpacePoints.push_back(top);
			BlendSpacePoint bottom = { Vec2(0.0f, -1.0f), 49 };
			_blendSpacePoints.push_back(bottom);

            BlendSpacePoint leftTop = { Vec2(-0.71f, 0.71f), 53 };
            _blendSpacePoints.push_back(leftTop);
            BlendSpacePoint rightTop = { Vec2(0.71f, 0.71f), 54 };
            _blendSpacePoints.push_back(rightTop);
            BlendSpacePoint leftBottom = { Vec2(-0.71f, -0.71f), 51 };
            _blendSpacePoints.push_back(leftBottom);
            BlendSpacePoint rightBottom = { Vec2(0.71f, -0.71f), 52 };
            _blendSpacePoints.push_back(rightBottom);
		}

		{
			BlendSpacePoint left = { Vec2(-2.0f, 0.0f), 44 };
            _blendSpacePoints.push_back(left);
            BlendSpacePoint right = { Vec2(2.0f, 0.0f), 45 };
            _blendSpacePoints.push_back(right);
            BlendSpacePoint top = { Vec2(0.0f, 2.0f), 37 };
            _blendSpacePoints.push_back(top);
            BlendSpacePoint bottom = { Vec2(0.0f, -2.0f), 36 };
            _blendSpacePoints.push_back(bottom);

            BlendSpacePoint leftTop = { Vec2(-1.41f, 1.41f), 57, 0.7f };
            _blendSpacePoints.push_back(leftTop);
            BlendSpacePoint rightTop = { Vec2(1.41f, 1.41f), 58, 0.7f };
            _blendSpacePoints.push_back(rightTop);
            BlendSpacePoint leftBottom = { Vec2(-1.41f, -1.41f), 55, 0.7f };
            _blendSpacePoints.push_back(leftBottom);
            BlendSpacePoint rightBottom = { Vec2(1.41f, -1.41f), 56, 0.7f };
            _blendSpacePoints.push_back(rightBottom);
		}
	}

	const BlendSpaceSample sample = EvaluateBlendSpace();
	UpdateBlendSpaceKeyframe(_tweenDesc.cur, sample);
}

void ModelAnimator::Update()
{
    const TweenDesc prevTweenDesc = _tweenDesc;
	UpdateTweenData();
	UpdateRootMotion(prevTweenDesc);

	if (_tweenDesc.cur.HasAnimation() && _tweenDesc.cur.isBlendSpace == false)
	{
		float leftTime = GetLeftTime(_tweenDesc.cur.animations[0]);
		if (leftTime < _tweenDesc.tweenDuration && _tweenDesc.next.HasAnimation() == false)
		{
			const BlendSpaceSample sample = EvaluateBlendSpace();
            UpdateBlendSpaceKeyframe(_tweenDesc.next, sample);
		}
	}
}

bool ModelAnimator::SetModel(ResourceRef<Model> model)
{
	Model* modelPtr = model.Resolve();
	if (modelPtr == nullptr || modelPtr->HasValidRenderResources() == false)
		return false;

	const AssetId oldMeshId = GetMeshId();
	_model = model;
	OnMeshChange(oldMeshId, GetMeshId());
	// 모델이 바뀌면 유효한 애니메이션 인덱스 범위도 달라질 수 있다.
	_blendSpaceTriangulationDirty = true;

	if (modelPtr->GetMaterialCount() > 0)
	{
        SetMaterial(modelPtr->GetMaterialByIndex(0));
	}

	_tweenDesc.ClearNextAnim();
	return true;
}

// 좌표와 애니메이션을 한 쌍으로 추가한다.
int ModelAnimator::AddBlendSpacePoint(const Vec2& position, int32 animIndex, float speed)
{
	Model* model = _model.Resolve();
	// 모델이 준비된 경우 유효한 애니메이션 인덱스로 제한한다.
	if (model != nullptr && model->GetAnimationCount() > 0)
		animIndex = clamp(animIndex, 0, static_cast<int32>(model->GetAnimationCount()) - 1);

	_blendSpacePoints.push_back({ position, animIndex, max(speed, 0.f) });
	_blendSpaceTriangulationDirty = true;
	return static_cast<int>(_blendSpacePoints.size()) - 1;
}

// 지정한 지점을 제거하고 에디터 선택 인덱스를 함께 보정한다.
bool ModelAnimator::RemoveBlendSpacePoint(int index)
{
	if (index < 0 || index >= static_cast<int>(_blendSpacePoints.size()))
		return false;

	_blendSpacePoints.erase(_blendSpacePoints.begin() + index);
	_blendSpaceTriangulationDirty = true;
	if (_selectedBlendSpacePoint == index)
		_selectedBlendSpacePoint = -1;
	else if (_selectedBlendSpacePoint > index)
		--_selectedBlendSpacePoint;
	return true;
}

// 모든 지점과 현재 선택 상태를 초기화한다.
void ModelAnimator::ClearBlendSpace()
{
	_blendSpacePoints.clear();
	_selectedBlendSpacePoint = -1;
	_blendSpaceTriangles.clear();
	_blendSpaceTriangulationDirty = false;
}

void ModelAnimator::RenderInstancing(InstancingBuffer& buffer, RenderTech renderTech)
{
    Model* model = _model.Resolve();
	if (model == nullptr)
		return;

    ModelMeshResource* mesh = model->GetMesh();
    if (mesh == nullptr)
        return;

	model->EnsureAnimationTexture();

	if (Super::Render(renderTech) == false)
		return;

    Material* material = GetMaterial().Resolve();
	Shader* shader = material->GetShader();

	//// GlobalData
	//_shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);
	//
	//// Light
	//auto lightObj = SCENE->GetCurrentScene()->GetLight();
	//if (lightObj)
	//	_shader->PushLightData(lightObj->GetLight()->GetLightDesc());

	// SRV를 통해 정보 전달
	shader->GetSRV("TransformMap")->SetResource(model->GetAnimationTransformSRV());

	// Bone
	BoneDesc boneDesc;

	const uint32 boneCount = mesh->GetBoneCount();
	//if (_showAnimationDebug)
	//{
	//
	//}
	//else
	//{
	//	_skinnedMesh.LoadBoneInfos(TIME->GetGameTime());
	//}
	//vector<SkinnedMesh::BoneInfo>& boneInfos = _skinnedMesh.GetBoneInfos();
    //for (uint32 i = 0; i < boneCount; i++)
    //{
	//	boneDesc.transforms[i] = boneInfos[i].FinalTransformation;
    //}
	for (uint32 i = 0; i < boneCount; i++)
	{
		shared_ptr<ModelBone> bone = mesh->GetBoneByIndex(i);
		boneDesc.transforms[i] = bone->globalMatrix;
	}
	shader->PushBoneData(boneDesc);

	const vector<shared_ptr<ModelMesh>>& meshes = mesh->GetMeshes();
	for (const shared_ptr<ModelMesh>& modelMesh : meshes)
	{
        Material* material = modelMesh->material.Resolve();
		if (material)
			material->Update();

		shader->GetScalar("BoneIndex")->SetInt(modelMesh->boneIndex);

		modelMesh->vertexBuffer->PushData();
		modelMesh->indexBuffer->PushData();
		buffer.PushData();
		shader->DrawIndexedInstanced(renderTech, _pass, modelMesh->indexBuffer->GetCount(), buffer.GetCount());
	}
}

void ModelAnimator::PlayAnimation(uint32 animIndex)
{
    Model* model = _model.Resolve();
    if (model == nullptr || model->GetAnimationCount() <= animIndex)
        return;

    _tweenDesc.next.SetSingleAnimation(animIndex);
}

void ModelAnimator::PlayAnimation(string animName)
{
    Model* model = _model.Resolve();
    if (model == nullptr)
        return;
    int animIndex = model->GetAnimationIndexByName(Utils::ToWString(animName));
    if (animIndex < 0)
        return;
	
	if (_tweenDesc.cur.GetSingleAnimationIndex() == animIndex)
		return;

    _tweenDesc.next.SetSingleAnimation(animIndex);
}

bool ModelAnimator::TryGetModelSocketWorldMatrix(const string& socketName, OUT Matrix& worldMatrix)
{
	Model* model = _model.Resolve();
	if (model == nullptr)
		return false;

	const ModelSocket* socket = model->GetModelSocketByName(socketName);
	ModelMeshResource* mesh = model->GetMesh();
	if (socket == nullptr || mesh == nullptr || socket->boneIndex < 0 ||
		socket->boneIndex >= static_cast<int32>(mesh->GetBoneCount()))
		return false;

	if (model->EnsureAnimationTexture() == false)
		return false;
	const vector<AnimTransform>& animTransforms = model->GetAnimationTransforms();

	auto sampleKeyframe = [&](const KeyframeDesc& keyframe, OUT Matrix& result)
		{
			ZeroMemory(&result, sizeof(result));
			bool sampled = false;
			const float* weights = &keyframe.blendWeights.x;
			for (int slot = 0; slot < MAX_BLEND_ANIMATIONS; ++slot)
			{
				const AnimationFrameDesc& frame = keyframe.animations[slot];
				if (frame.animIndex < 0 || frame.animIndex >= static_cast<int32>(animTransforms.size()) || weights[slot] <= 0.f)
					continue;
				ModelAnimation* animation = model->GetAnimationByIndex(frame.animIndex);
				if (animation == nullptr || frame.curFrame >= animation->GetFrameCount() || frame.nextFrame >= animation->GetFrameCount())
					continue;

				const AnimTransform& transforms = animTransforms[frame.animIndex];
				const Matrix sampledMatrix = Matrix::Lerp(
					transforms.transforms[frame.curFrame][socket->boneIndex],
					transforms.transforms[frame.nextFrame][socket->boneIndex],
					frame.ratio);
				float* destination = &result._11;
				const float* source = &sampledMatrix._11;
				for (int valueIndex = 0; valueIndex < 16; ++valueIndex)
					destination[valueIndex] += source[valueIndex] * weights[slot];
				sampled = true;
			}
			return sampled;
		};

	Matrix boneFinal;
	if (!sampleKeyframe(_tweenDesc.cur, OUT boneFinal))
		return false;
	Matrix nextBoneFinal;
	if (sampleKeyframe(_tweenDesc.next, OUT nextBoneFinal))
		boneFinal = Matrix::Lerp(boneFinal, nextBoneFinal, _tweenDesc.tweenRatio);

	const Matrix boneGlobal = mesh->GetBoneByIndex(socket->boneIndex)->offsetMatrix.Invert() * boneFinal;
	worldMatrix = socket->localMatrix * boneGlobal * GetTransform()->GetWorldMatrix();
	return true;
}

bool ModelAnimator::OnGUI()
{
	bool changed = false;
    changed |= Super::OnGUI();
	ImGui::Separator();
	ResourceRef<Model> modelRef = _model;
	if (OnGUIUtils::DrawResourceRef("Model", modelRef))
		changed |= SetModel(modelRef);
    changed |= OnGUIUtils::DrawFloat("Tween Duration", &_tweenDesc.tweenDuration, 0.1f);
    changed |= OnGUIUtils::DrawFloat("Tween Ratio", &_tweenDesc.tweenRatio, 0.01f);
    changed |= OnGUIUtils::DrawFloat("Tween SumTime", &_tweenDesc.tweenSumTime, 0.01f);

    ImGui::Separator();
	if (ImGui::TreeNode("Current Animation"))
	{
        for (int i = 0; i < MAX_BLEND_ANIMATIONS; ++i)
        {
            ImGui::PushID(i);
            ImGui::Text("Slot %d", i);
            changed |= OnGUIUtils::DrawInt32("Anim Index", &_tweenDesc.cur.animations[i].animIndex, 1.f);
            changed |= OnGUIUtils::DrawUInt32("Cur Frame Index", &_tweenDesc.cur.animations[i].curFrame, 1.f);
            changed |= OnGUIUtils::DrawUInt32("Next Frame Index", &_tweenDesc.cur.animations[i].nextFrame, 1.f);
            changed |= OnGUIUtils::DrawFloat("Cur Ratio", &_tweenDesc.cur.animations[i].ratio, 0.1f);
			ImGui::PopID();
			ImGui::Separator();
        }
		ImGui::TreePop();
	}
    if (ImGui::TreeNode("Next Animation"))
    {
        for (int i = 0; i < MAX_BLEND_ANIMATIONS; ++i)
        {
            ImGui::PushID(i);
            ImGui::Text("Slot %d", i);
            changed |= OnGUIUtils::DrawInt32("Anim Index", &_tweenDesc.next.animations[i].animIndex, 1.f);
            changed |= OnGUIUtils::DrawUInt32("Cur Frame Index", &_tweenDesc.next.animations[i].curFrame, 1.f);
            changed |= OnGUIUtils::DrawUInt32("Next Frame Index", &_tweenDesc.next.animations[i].nextFrame, 1.f);
            changed |= OnGUIUtils::DrawFloat("Cur Ratio", &_tweenDesc.next.animations[i].ratio, 0.1f);
            ImGui::PopID();
            ImGui::Separator();
        }
        ImGui::TreePop();
    }
	changed |= OnGUIUtils::DrawFloat("Sum Time", &_tweenDesc.cur.sumTime, 1.f);
	changed |= OnGUIUtils::DrawFloat("Ratio", &_tweenDesc.cur.animations[0].ratio, 1.f);
    ImGui::Separator();

	changed |= OnGUIUtils::DrawFloat("Anim Speed", &_tweenDesc.speed, 0.1f);

	Model* model = _model.Resolve();
	if (model != nullptr && model->GetAnimationCount() > 0)
	{
		const int animCount = model->GetAnimationCount();
		string nextAnimName = Utils::ToString(model->GetAnimationByIndex(_nextAnimIndex)->GetName());
		nextAnimName = to_string(_nextAnimIndex) + " : " + nextAnimName;
		if (ImGui::BeginCombo("Animation", nextAnimName.c_str()))
		{
			for (int animationIndex = 0; animationIndex < animCount; ++animationIndex)
			{
				const string name = to_string(animationIndex) + " : " +
					Utils::ToString(model->GetAnimationByIndex(animationIndex)->GetName());
				if (ImGui::Selectable(name.c_str(), _nextAnimIndex == animationIndex))
				{
					_nextAnimIndex = animationIndex;
					_tweenDesc.next.SetSingleAnimation(_nextAnimIndex);
					changed = true;
				}
				if (_nextAnimIndex == animationIndex)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if (ImGui::Button("To BlendSpace"))
		{
			const BlendSpaceSample sample = EvaluateBlendSpace();
			UpdateBlendSpaceKeyframe(_tweenDesc.next, sample);
			changed = true;
		}
	}

	ImGui::SeparatorText("XY Blend Space");
	changed |= DrawBlendSpaceEditor();
	bool showAnimDebugChanged = OnGUIUtils::DrawBool("Show Animation Debug", &_showAnimationDebug);
    changed |= showAnimDebugChanged;
	if (showAnimDebugChanged && _showAnimationDebug == false)
	{
        EDITOR->GetSceneView()->ClearTransformGizmoOverride();
	}

	if (_showAnimationDebug)
		changed |= DrawDebugWindow();

	return changed;
}

bool ModelAnimator::TryInitialize()
{
	if(_initialized)
        return true;

    Model* modelPtr = _model.Resolve();
    if (modelPtr == nullptr)
        return false;

	auto& materials = modelPtr->GetMaterials();
    bool hasValidMaterial = false;
	for (auto& materialRef : materials)
	{
        Material* material = materialRef.Resolve();
        if (material == nullptr)
            continue;

		if (material->GetShader() == nullptr)
			material->SetShader(RESOURCES->GetDefaultShader());

        hasValidMaterial = true;
	}

    if (hasValidMaterial == false)
        return false;

    _initialized = true;
    return true;
}

void ModelAnimator::OnInspectorFocusLost()
{
    Super::OnInspectorFocusLost();
    _showAnimationDebug = false;
    EDITOR->GetSceneView()->ClearTransformGizmoOverride();
}

void ModelAnimator::UpdateBlendSpaceTriangulation()
{
	Model* model = _model.Resolve();
	const int animationCount = model != nullptr ? static_cast<int>(model->GetAnimationCount()) : 0;

	// 모델의 애니메이션 개수가 바뀐 경우에도 유효한 점 목록을 다시 계산한다.
	if (_blendSpaceCachedAnimationCount != animationCount)
		_blendSpaceTriangulationDirty = true;

	if (!_blendSpaceTriangulationDirty)
		return;

	vector<Geometry2D::Point> points;
	points.reserve(_blendSpacePoints.size());
	vector<int> validIndices;
	for (int i = 0; i < static_cast<int>(_blendSpacePoints.size()); ++i)
	{
		const BlendSpacePoint& point = _blendSpacePoints[i];
		points.emplace_back(point.position.x, point.position.y);
		if (point.animIndex >= 0 && point.animIndex < animationCount)
		{
			validIndices.push_back(i);
		}
	}

	_blendSpaceTriangles.clear();
	int lineStart = -1;
	int lineEnd = -1;
	if (AreBlendSpacePointsCollinear(points, validIndices, lineStart, lineEnd))
	{
		_blendSpaceCachedAnimationCount = animationCount;
		_blendSpaceTriangulationDirty = false;
		return;
	}

	const vector<Geometry2D::TriangleIndices> triangles =
		Geometry2D::DelaunayTriangulate(points, validIndices);

	_blendSpaceTriangles.reserve(triangles.size());
	for (const Geometry2D::TriangleIndices& triangle : triangles)
		_blendSpaceTriangles.push_back(triangle.indices);

	_blendSpaceCachedAnimationCount = animationCount;
	_blendSpaceTriangulationDirty = false;
}

ModelAnimator::BlendSpaceSample ModelAnimator::EvaluateBlendSpace()
{
	// 유효한 샘플을 찾지 못하면 요청한 입력 좌표와 0 가중치를 반환한다.
	BlendSpaceSample result;
	result.sampledPosition = _blendSpaceInput;

	Model* model = _model.Resolve();
	if (model == nullptr)
		return result;

	vector<Geometry2D::Point> points;
	points.reserve(_blendSpacePoints.size());
	vector<int> validIndices;
	// 좌표는 모두 보존하되, 실제 모델에 존재하는 애니메이션을 가진 점만 계산에 사용한다.
	for (int i = 0; i < static_cast<int>(_blendSpacePoints.size()); ++i)
	{
		const BlendSpacePoint& point = _blendSpacePoints[i];
		points.emplace_back(point.position.x, point.position.y);
		if (point.animIndex >= 0 && point.animIndex < static_cast<int>(model->GetAnimationCount()))
			validIndices.push_back(i);
	}

	if (validIndices.empty())
		return result;

	const Geometry2D::Point query(_blendSpaceInput.x, _blendSpaceInput.y);
	int lineStart = -1;
	int lineEnd = -1;
	const bool isCollinear = AreBlendSpacePointsCollinear(
		points, validIndices, lineStart, lineEnd);

	if (validIndices.size() == 1 || lineStart == lineEnd)
	{
		// 점이 하나뿐이거나 모든 점의 좌표가 같으면 첫 샘플을 사용한다.
		result.pointIndices[0] = validIndices[0];
		result.weights[0] = 1.0f;
		result.sampledPosition = _blendSpacePoints[validIndices[0]].position;
		return result;
	}

	if (isCollinear)
	{
		// 주축을 따라 정렬한 뒤 인접한 선분만 비교한다. 모든 점 쌍을 비교하면
		// 긴 선분이 중간 샘플을 건너뛰어 선택될 수 있다.
		const Geometry2D::Point line = points[lineEnd] - points[lineStart];
		vector<int> orderedIndices = validIndices;
		sort(orderedIndices.begin(), orderedIndices.end(), [&](int lhs, int rhs)
		{
			return Geometry2D::Dot(points[lhs] - points[lineStart], line) <
				Geometry2D::Dot(points[rhs] - points[lineStart], line);
		});

		float closestDistanceSquared = FLT_MAX;
		for (size_t i = 1; i < orderedIndices.size(); ++i)
		{
			const int first = orderedIndices[i - 1];
			const int second = orderedIndices[i];
			const Geometry2D::Point edge = points[second] - points[first];
			const float edgeLengthSquared = Geometry2D::LengthSquared(edge);
			if (edgeLengthSquared <= Geometry2D::DefaultEpsilon * Geometry2D::DefaultEpsilon)
				continue;

			const Geometry2D::Point closest =
				Geometry2D::ClosestPointOnSegment(query, points[first], points[second]);
			const float distanceSquared = Geometry2D::LengthSquared(query - closest);
			if (distanceSquared >= closestDistanceSquared)
				continue;

			const float t = clamp(
				Geometry2D::Dot(closest - points[first], edge) / edgeLengthSquared,
				0.0f, 1.0f);
			closestDistanceSquared = distanceSquared;
			result.pointIndices = { first, second, -1 };
			result.weights = { 1.0f - t, t, 0.0f };
			result.sampledPosition = Vec2(closest.x, closest.y);
		}

		return result;
	}

	// 삼각분할은 dirty 상태일 때만 갱신하고 평소에는 캐시된 인덱스를 사용한다.
	UpdateBlendSpaceTriangulation();

	float bestDistanceSquared = FLT_MAX;
	Geometry2D::TriangleIndices bestTriangle;
	Geometry2D::Point bestPoint;
	bool foundTriangle = false;

	// 입력이 삼각망 밖에 있으면 각 삼각형의 최근접점을 비교해 외곽에 투영한다.
	for (const array<int, 3>& triangle : _blendSpaceTriangles)
	{
		const Geometry2D::Point closest = Geometry2D::ClosestPointOnTriangle(
			query,
			points[triangle[0]],
			points[triangle[1]],
			points[triangle[2]]);
		const float distanceSquared = Geometry2D::LengthSquared(query - closest);
		if (distanceSquared < bestDistanceSquared)
		{
			bestDistanceSquared = distanceSquared;
			bestTriangle.indices = triangle;
			bestPoint = closest;
			foundTriangle = true;
		}
	}

	if (foundTriangle)
	{
		// 선택된 삼각형 안에서 세 꼭짓점의 미리보기 가중치를 계산한다.
		Geometry2D::BarycentricCoordinates barycentric;
		if (Geometry2D::TryGetBarycentricCoordinates(
			bestPoint,
			points[bestTriangle.indices[0]],
			points[bestTriangle.indices[1]],
			points[bestTriangle.indices[2]],
			barycentric))
		{
			result.pointIndices = bestTriangle.indices;
			result.weights =
			{
				max(0.0f, barycentric.a),
				max(0.0f, barycentric.b),
				max(0.0f, barycentric.c)
			};
			const float weightSum = result.weights[0] + result.weights[1] + result.weights[2];
			// 부동소수점 오차를 제거해 세 가중치의 합을 1로 맞춘다.
			if (weightSum > 0.0f)
			{
				for (float& weight : result.weights)
					weight /= weightSum;
			}
			result.sampledPosition = Vec2(bestPoint.x, bestPoint.y);
			return result;
		}
	}

	// 삼각분할에 실패한 경우에는
	// 입력과 가장 가까운 두 점의 선분 위에서 가중치를 계산한다.
	bestDistanceSquared = FLT_MAX;
	for (size_t i = 0; i < validIndices.size(); ++i)
	{
		for (size_t j = i + 1; j < validIndices.size(); ++j)
		{
			const int first = validIndices[i];
			const int second = validIndices[j];
			const Geometry2D::Point closest =
				Geometry2D::ClosestPointOnSegment(query, points[first], points[second]);
			const float distanceSquared = Geometry2D::LengthSquared(query - closest);
			if (distanceSquared >= bestDistanceSquared)
				continue;

			const Geometry2D::Point edge = points[second] - points[first];
			const float edgeLengthSquared = Geometry2D::LengthSquared(edge);
			const float t = edgeLengthSquared > Geometry2D::DefaultEpsilon
				? clamp(Geometry2D::Dot(closest - points[first], edge) / edgeLengthSquared, 0.0f, 1.0f)
				: 0.0f;
			bestDistanceSquared = distanceSquared;
			result.pointIndices = { first, second, -1 };
			result.weights = { 1.0f - t, t, 0.0f };
			result.sampledPosition = Vec2(closest.x, closest.y);
		}
	}

	return result;
}

bool ModelAnimator::DrawBlendSpaceEditor()
{
	bool changed = false;

	// 입력 좌표와 그래프에 표시할 XY 축 범위를 편집한다.
	changed |= ImGui::DragFloat2("Input", &_blendSpaceInput.x, 0.01f);
	changed |= ImGui::DragFloat2("Axis Min", &_blendSpaceMin.x, 0.01f);
	changed |= ImGui::DragFloat2("Axis Max", &_blendSpaceMax.x, 0.01f);
	_blendSpaceMax.x = max(_blendSpaceMax.x, _blendSpaceMin.x + 0.001f);
	_blendSpaceMax.y = max(_blendSpaceMax.y, _blendSpaceMin.y + 0.001f);

	Model* model = _model.Resolve();
	const int animationCount = model != nullptr ? static_cast<int>(model->GetAnimationCount()) : 0;

	// 현재 입력 좌표에 새 지점을 추가하거나 전체 지점을 삭제한다.
	if (ImGui::Button("Add Point") && animationCount > 0)
	{
		_selectedBlendSpacePoint = AddBlendSpacePoint(_blendSpaceInput, 0);
		changed = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear Points") && !_blendSpacePoints.empty())
	{
		ClearBlendSpace();
		changed = true;
	}

	// 각 지점의 좌표와 연결할 애니메이션을 편집한다.
	int removeIndex = -1;
	if (ImGui::TreeNode("BlendSpacePoints", "Blend Space Points (%d)", static_cast<int>(_blendSpacePoints.size())))
	{
		for (int i = 0; i < static_cast<int>(_blendSpacePoints.size()); ++i)
		{
			BlendSpacePoint& point = _blendSpacePoints[i];
			ImGui::PushID(i);
			const bool open = ImGui::TreeNodeEx(
				"Point",
				(_selectedBlendSpacePoint == i ? ImGuiTreeNodeFlags_Selected : 0),
				"Point %d", i);
			if (ImGui::IsItemClicked())
				_selectedBlendSpacePoint = i;
			ImGui::SameLine();
			if (ImGui::SmallButton("Remove"))
				removeIndex = i;

			if (open)
			{
				if (ImGui::DragFloat2("Coordinate", &point.position.x, 0.01f))
				{
					_blendSpaceTriangulationDirty = true;
					changed = true;
				}
				const char* preview = "<invalid>";
				string previewStorage;
				if (model != nullptr && point.animIndex >= 0 && point.animIndex < animationCount)
				{
					previewStorage = to_string(point.animIndex) + " : " +
						Utils::ToString(model->GetAnimationByIndex(point.animIndex)->GetName());
					preview = previewStorage.c_str();
				}
				if (ImGui::BeginCombo("Animation", preview))
				{
					for (int animationIndex = 0; animationIndex < animationCount; ++animationIndex)
					{
						const string name = to_string(animationIndex) + " : " +
							Utils::ToString(model->GetAnimationByIndex(animationIndex)->GetName());
						if (ImGui::Selectable(name.c_str(), point.animIndex == animationIndex))
						{
							point.animIndex = animationIndex;
							_blendSpaceTriangulationDirty = true;
							changed = true;
						}
						if (point.animIndex == animationIndex)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				if (ImGui::DragFloat("Speed", &point.speed, 0.01f, 0.f, FLT_MAX))
				{
					point.speed = max(point.speed, 0.f);
					changed = true;
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		ImGui::TreePop();
	}
	if (removeIndex >= 0)
	{
		RemoveBlendSpacePoint(removeIndex);
		changed = true;
	}

	// 블렌드 스페이스 그래프가 그려질 ImGui 캔버스를 준비한다.
    const float canvasWidth = clamp(ImGui::GetContentRegionAvail().x - 20.f, 100.f, 260.f);
	const ImVec2 canvasSize(canvasWidth, canvasWidth);
	const ImVec2 canvasMin = ImGui::GetCursorScreenPos();
	const ImVec2 canvasMax(canvasMin.x + canvasSize.x, canvasMin.y + canvasSize.y);
	ImGui::InvisibleButton("##BlendSpaceCanvas", canvasSize);
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(24, 27, 32, 255));
	drawList->AddRect(canvasMin, canvasMax, IM_COL32(100, 110, 125, 255));

	// 블렌드 좌표와 ImGui 화면 좌표를 서로 변환한다.
	auto ToScreen = [&](const Vec2& point)
	{
		const float u = (point.x - _blendSpaceMin.x) / (_blendSpaceMax.x - _blendSpaceMin.x);
		const float v = (point.y - _blendSpaceMin.y) / (_blendSpaceMax.y - _blendSpaceMin.y);
		return ImVec2(
			canvasMin.x + clamp(u, 0.0f, 1.0f) * canvasSize.x,
			canvasMax.y - clamp(v, 0.0f, 1.0f) * canvasSize.y);
	};
	auto FromScreen = [&](const ImVec2& point)
	{
		const float u = clamp((point.x - canvasMin.x) / canvasSize.x, 0.0f, 1.0f);
		const float v = clamp((canvasMax.y - point.y) / canvasSize.y, 0.0f, 1.0f);
		return Vec2(
			_blendSpaceMin.x + u * (_blendSpaceMax.x - _blendSpaceMin.x),
			_blendSpaceMin.y + v * (_blendSpaceMax.y - _blendSpaceMin.y));
	};

	// 현재 축 범위 안에 원점이 포함되면 X/Y 기준선을 표시한다.
	if (_blendSpaceMin.x <= 0.0f && _blendSpaceMax.x >= 0.0f)
	{
		const float x = ToScreen(Vec2(0.0f, 0.0f)).x;
		drawList->AddLine(ImVec2(x, canvasMin.y), ImVec2(x, canvasMax.y), IM_COL32(65, 70, 80, 255));
	}
	if (_blendSpaceMin.y <= 0.0f && _blendSpaceMax.y >= 0.0f)
	{
		const float y = ToScreen(Vec2(0.0f, 0.0f)).y;
		drawList->AddLine(ImVec2(canvasMin.x, y), ImVec2(canvasMax.x, y), IM_COL32(65, 70, 80, 255));
	}

	// 유효한 지점으로 생성해 둔 Delaunay 삼각망 캐시를 표시한다.
	UpdateBlendSpaceTriangulation();
	for (const array<int, 3>& triangle : _blendSpaceTriangles)
	{
		const ImVec2 a = ToScreen(_blendSpacePoints[triangle[0]].position);
		const ImVec2 b = ToScreen(_blendSpacePoints[triangle[1]].position);
		const ImVec2 c = ToScreen(_blendSpacePoints[triangle[2]].position);
		drawList->AddTriangleFilled(a, b, c, IM_COL32(45, 90, 125, 55));
		drawList->AddTriangle(a, b, c, IM_COL32(75, 155, 210, 180), 1.5f);
	}

	// 지점과 현재 선택 상태를 원으로 표시한다.
	const BlendSpaceSample sample = EvaluateBlendSpace();
	for (int i = 0; i < static_cast<int>(_blendSpacePoints.size()); ++i)
	{
		const ImVec2 position = ToScreen(_blendSpacePoints[i].position);
		const bool selected = i == _selectedBlendSpacePoint;
		drawList->AddCircleFilled(position, selected ? 7.0f : 5.0f,
			selected ? IM_COL32(255, 190, 55, 255) : IM_COL32(215, 220, 230, 255));
		drawList->AddText(ImVec2(position.x + 7.0f, position.y - 8.0f),
			IM_COL32(225, 225, 225, 255), to_string(i).c_str());
	}

	// 요청 입력은 주황색, 삼각망에 투영된 샘플 위치는 초록색으로 표시한다.
	const ImVec2 sampledScreen = ToScreen(sample.sampledPosition);
	const ImVec2 inputScreen = ToScreen(_blendSpaceInput);
	drawList->AddLine(inputScreen, sampledScreen, IM_COL32(255, 170, 50, 150), 1.0f);
	drawList->AddCircle(sampledScreen, 6.0f, IM_COL32(80, 220, 150, 255), 0, 2.0f);
	drawList->AddCircleFilled(inputScreen, 3.5f, IM_COL32(255, 120, 70, 255));

	// 캔버스를 드래그하면 화면 좌표를 블렌드 입력 좌표로 변환한다.
	if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		_blendSpaceInput = FromScreen(ImGui::GetIO().MousePos);
		changed = true;
	}

	// 현재 미리보기에 참여하는 지점별 가중치를 텍스트로 표시한다.
	ImGui::TextDisabled("Drag in the graph to set input. Green: sampled position, orange: requested input.");
	for (int i = 0; i < 3; ++i)
	{
		if (sample.pointIndices[i] < 0 || sample.weights[i] <= 0.0001f)
			continue;
		const BlendSpacePoint& point = _blendSpacePoints[sample.pointIndices[i]];
		ImGui::Text("Point %d / Anim %d: %.1f%%",
			sample.pointIndices[i], point.animIndex, sample.weights[i] * 100.0f);
	}

	return changed;
}

void ModelAnimator::UpdateRootMotion(const TweenDesc prevTweenDesc)
{
    auto updateRootMotion = [&](const KeyframeDesc& keyframe, Model* model)
        {
			// 모델이나 일반 애니메이션이 유효하지 않으면 루트모션을 계산하지 않는다.
			if (model == nullptr || keyframe.HasAnimation() == false || keyframe.isBlendSpace == true)
				return;
			if (model->EnsureAnimationTexture() == false)
				return;
			const vector<AnimTransform>& animTransforms = model->GetAnimationTransforms();

            const int animIndex = keyframe.animations[0].animIndex;
			// 잘못된 애니메이션 인덱스로 캐시를 참조하지 않도록 방어한다.
			if (animIndex < 0 || model->GetAnimationCount() <= animIndex || animTransforms.size() <= static_cast<size_t>(animIndex))
                return;

			ModelAnimation* animation = model->GetAnimationByIndex(animIndex);
			// 프레임이 없는 애니메이션은 샘플링할 수 없다.
			if (animation == nullptr || animation->GetFrameCount() == 0)
				return;

			const AnimationClipImportSetting& importSetting = animation->GetAnimationClipImportSetting();
            if (importSetting.extractRootMotion == false)
                return;

			bool containInPrevTween = false;

            const KeyframeDesc* prevKeyframe = &prevTweenDesc.cur;
			if (prevKeyframe->HasAnimation() && prevKeyframe->isBlendSpace == false && prevKeyframe->animations[0].animIndex == animIndex)
			{
                containInPrevTween = true;
			}
			else
			{
                prevKeyframe = &prevTweenDesc.next;
				if (prevKeyframe->HasAnimation() && prevKeyframe->isBlendSpace == false && prevKeyframe->animations[0].animIndex == animIndex)
				{
                    containInPrevTween = true;
				}
			}

			if (containInPrevTween == false)
				return;
            const AnimationFrameDesc& prevAnimFrame = prevKeyframe->animations[0];
            const AnimationFrameDesc& curAnimFrame = keyframe.animations[0];
			const uint32 frameCount = animation->GetFrameCount();

            const AnimTransform& animTransform = animTransforms[animIndex];
			// 마지막 프레임에서 0번 프레임으로 보간하면 루트가 원점으로 되감기므로
			// 루프 경계에서는 마지막 프레임의 루트 위치를 그대로 유지한다.
			auto sampleRootMotion = [&](const AnimationFrameDesc& frame)
				{
					const Matrix& rootMotion1 = animTransform.rootTransforms[frame.curFrame];
					if (frame.nextFrame < frame.curFrame)
						return rootMotion1;

					const Matrix& rootMotion2 = animTransform.rootTransforms[frame.nextFrame];
					return Matrix::Lerp(rootMotion1, rootMotion2, frame.ratio);
				};

			Matrix prevRootMotion = sampleRootMotion(prevAnimFrame);
			Matrix curRootMotion = sampleRootMotion(curAnimFrame);
			Matrix rootMotionDelta;
			// sumTime이 감소했다면 이번 업데이트에서 애니메이션 루프를 통과한 것이다.
			// 이전 위치에서 클립 끝까지, 클립 시작에서 현재 위치까지의 이동량을 이어 붙인다.
			if (keyframe.sumTime < prevKeyframe->sumTime)
			{
				const Matrix& startRootMotion = animTransform.rootTransforms[0];
				const Matrix& endRootMotion = animTransform.rootTransforms[frameCount - 1];
				const Matrix deltaToEnd = endRootMotion * prevRootMotion.Invert();
				const Matrix deltaFromStart = curRootMotion * startRootMotion.Invert();
				rootMotionDelta = deltaFromStart * deltaToEnd;
			}
			else
			{
				rootMotionDelta = curRootMotion * prevRootMotion.Invert();
			}

            Matrix gameObjectMatrix = GetTransform()->GetWorldMatrix();
            rootMotionDelta = rootMotionDelta * gameObjectMatrix;
			GetTransform()->SetWorldMatrix(rootMotionDelta);
        };

	Model* model = _model.Resolve();

    updateRootMotion(_tweenDesc.cur, model);
    updateRootMotion(_tweenDesc.next, model);
}

void ModelAnimator::UpdateTweenData()
{
	if (_showAnimationDebug)
		return;

	Model* model = _model.Resolve();
	if (model == nullptr || model->GetAnimationCount() == 0)
		return;

	UpdateBlendSpaceTriangulation();
	const BlendSpaceSample sample = EvaluateBlendSpace();

	// 현재 애니메이션 업데이트
    if (_tweenDesc.cur.isBlendSpace)
        UpdateBlendSpaceKeyframe(_tweenDesc.cur, sample);
    else
        UpdateRegularKeyframe(_tweenDesc.cur);

    // 다음 애니메이션이 존재하면 업데이트 및 트윈 비율 계산
	if (_tweenDesc.next.HasAnimation())
	{
		_tweenDesc.tweenSumTime += DT;
		_tweenDesc.tweenRatio = clamp(_tweenDesc.tweenSumTime / _tweenDesc.tweenDuration, 0.f, 1.f);
		if (_tweenDesc.tweenRatio >= 1.f)
		{
			_tweenDesc.cur = _tweenDesc.next;
			_tweenDesc.ClearNextAnim();
			return;
		}

		if (_tweenDesc.next.isBlendSpace)
			UpdateBlendSpaceKeyframe(_tweenDesc.next, sample);
		else
			UpdateRegularKeyframe(_tweenDesc.next);
	}
}

void ModelAnimator::UpdateRegularKeyframe(KeyframeDesc& keyframe)
{
	Model* model = _model.Resolve();
	if (model == nullptr || model->GetAnimationCount() == 0)
		return;

	AnimationFrameDesc& frame = keyframe.animations[0];
	frame.animIndex = clamp(frame.animIndex, 0, static_cast<int>(model->GetAnimationCount()) - 1);
	keyframe.blendWeights = Vec3(1.f, 0.f, 0.f);
	keyframe.animations[1] = {};
	keyframe.animations[2] = {};

	ModelAnimation* animation = model->GetAnimationByIndex(frame.animIndex);
	if (animation == nullptr || animation->GetFrameCount() == 0 || animation->GetFrameRate() <= 0.f)
		return;

	// 일반 애니메이션은 기존 speed 배율로 독립 재생한다.
	const uint32 prevFrame = frame.curFrame;
	const bool animationStarted = frame.curFrame == 0 && frame.nextFrame == 0;
	const float duration = animation->GetFrameCount() / animation->GetFrameRate();
	keyframe.sumTime = fmod(keyframe.sumTime + DT * max(_tweenDesc.speed, 0.f), duration);
	const float framePosition = keyframe.sumTime * animation->GetFrameRate();
	frame.curFrame = static_cast<uint32>(framePosition) % animation->GetFrameCount();
	frame.nextFrame = (frame.curFrame + 1) % animation->GetFrameCount();
	frame.ratio = framePosition - floor(framePosition);

	for (const AnimationEvent& animationEvent : animation->GetAnimationEvents())
	{
		if (animationEvent.eventName.empty() || animationEvent.frame >= animation->GetFrameCount())
			continue;

		const uint32 animFrameTarget = animationEvent.frame;
		const bool framePassed = prevFrame <= frame.curFrame
			? animFrameTarget > prevFrame && animFrameTarget <= frame.curFrame
			: animFrameTarget > prevFrame || animFrameTarget <= frame.curFrame;
		if (!framePassed && !(animationStarted && animationEvent.frame == 0))
			continue;

		for (const ComponentRef<MonoBehaviour>& scriptRef : GetGameObject()->GetScripts())
		{
			MonoBehaviour* script = scriptRef.Resolve();
			if (script != nullptr)
				script->OnAnimationEvent(animationEvent);
		}
	}
}

void ModelAnimator::UpdateBlendSpaceKeyframe(KeyframeDesc& keyframe, const BlendSpaceSample& sample)
{
	Model* model = _model.Resolve();
	if (model == nullptr)
		return;

    keyframe.isBlendSpace = true;

	// 포즈는 같은 정규화 위상에서 샘플링하되, 위상 진행 속도는 각 클립의
	// 원래 사이클 빈도(frameRate / cycleFrameCount)를 가중 보간한다.
	float blendedCycleFrequency = 0.f;
	for (int i = 0; i < MAX_BLEND_ANIMATIONS; ++i)
	{
		const int pointIndex = sample.pointIndices[i];
		if (pointIndex < 0 || sample.weights[i] <= 0.f)
			continue;

		const BlendSpacePoint& point = _blendSpacePoints[pointIndex];
		const int animIndex = point.animIndex;
		ModelAnimation* animation = model->GetAnimationByIndex(animIndex);
		if (animation == nullptr || animation->GetFrameCount() <= 1 || animation->GetFrameRate() <= 0.f)
			continue;

		const float cycleFrameCount = static_cast<float>(animation->GetFrameCount() - 1);
		blendedCycleFrequency += sample.weights[i] * max(point.speed, 0.f) *
			animation->GetFrameRate() / cycleFrameCount;
	}

	keyframe.sumTime = fmod(
		keyframe.sumTime + DT * max(_tweenDesc.speed, 0.f) * blendedCycleFrequency,
		1.f);
	const float normalizedTime = keyframe.sumTime;
	keyframe.blendWeights = Vec3(sample.weights[0], sample.weights[1], sample.weights[2]);

	for (int i = 0; i < MAX_BLEND_ANIMATIONS; ++i)
	{
		AnimationFrameDesc& frame = keyframe.animations[i];
		const int pointIndex = sample.pointIndices[i];
		if (pointIndex < 0 || sample.weights[i] <= 0.f)
		{
			frame = {};
			continue;
		}

		frame.animIndex = _blendSpacePoints[pointIndex].animIndex;
		ModelAnimation* animation = model->GetAnimationByIndex(frame.animIndex);
		if (animation == nullptr || animation->GetFrameCount() <= 1)
		{
			frame = {};
			continue;
		}
		
		// 처음과 끝 애니메이션은 동일하므로 끝 애니메이션은 생략
        const uint32 modFrameCount = animation->GetFrameCount() - 1;

		const float framePosition = normalizedTime * modFrameCount;
		frame.curFrame = static_cast<uint32>(framePosition) % modFrameCount;
		frame.nextFrame = (frame.curFrame + 1) % modFrameCount;
		frame.ratio = framePosition - floor(framePosition);
	}
}

bool ModelAnimator::DrawDebugWindow()
{
	bool changed = false;
    Model* model = _model.Resolve();
    if (model == nullptr)
        return false;

    bool wasOpen = _showAnimationDebug;
	string windowTitle = "Animation Debug";
	ImGui::SetNextWindowSize(ImVec2(1050.0f, 700.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(windowTitle.c_str(), &_showAnimationDebug))
	{
		EDITOR->GetSceneView()->ClearTransformGizmoOverride();
		ImGui::End();
		return false;
	}

	const uint32 animationCount = model->GetAnimationCount();
	if (animationCount == 0)
	{
		ImGui::TextDisabled("No animation is loaded in SkinnedMesh.");
		ImGui::End();
		return false;
	}

	ModelMeshResource* mesh = model->GetMesh();
	if (mesh == nullptr || mesh->GetBoneCount() == 0)
	{
		ImGui::TextDisabled("No skeleton is loaded in Model.");
		ImGui::End();
		return false;
	}
	if (model->EnsureAnimationTexture() == false)
	{
		ImGui::TextDisabled("Failed to create the animation transform texture.");
		ImGui::End();
		return false;
	}
	const vector<AnimTransform>& animTransforms = model->GetAnimationTransforms();

	_debugAnimationIndex = clamp(_debugAnimationIndex, 0, (int)animationCount - 1);
	ModelAnimation* animation = model->GetAnimationByIndex(_debugAnimationIndex);
	if (animation == nullptr || animation->GetFrameCount() == 0)
	{
		ImGui::TextDisabled("The selected animation has no frames.");
		ImGui::End();
		return false;
	}
	string animationName = Utils::ToString(animation->GetName());
	if (ImGui::BeginCombo("Animation", animationName.c_str()))
	{
		for (uint32 i = 0; i < animationCount; ++i)
		{
			const bool selected = i == (uint32)_debugAnimationIndex;
			string name = Utils::ToString(model->GetAnimationByIndex(i)->GetName());
			name = to_string(i) + " : " + name;
			if (ImGui::Selectable(name.c_str(), selected))
			{
				_debugAnimationIndex = (int)i;
				_debugFrameIndex = 0;
				_debugBoneIndex = 0;
				_debugSocketIndex = -1;
                _lastDebugFrameIndex = -1;
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	const uint32 frameCount = animation->GetFrameCount();
	_debugFrameIndex = clamp(_debugFrameIndex, 0, max(0, (int)frameCount - 1));
	ImGui::SetNextItemWidth(500.0f);
	ImGui::SliderInt("Frame", &_debugFrameIndex, 0, max(0, (int)frameCount - 1));

	// 디버그 모드에서는 첫 번째 슬롯만 사용하는 일반 KeyframeDesc로 고정한다.
	_tweenDesc.cur.SetSingleAnimation(_debugAnimationIndex);
	_tweenDesc.cur.animations[0].curFrame = _debugFrameIndex;
	_tweenDesc.cur.animations[0].nextFrame = (_debugFrameIndex + 1) % frameCount;
	_tweenDesc.cur.animations[0].ratio = 0.0f;
    _tweenDesc.cur.sumTime = 0.0f;

	const float ticksPerSecond = animation->GetFrameRate();
	const float timeSeconds = ticksPerSecond > 0.0f ? _debugFrameIndex / ticksPerSecond : 0.0f;

	ImGui::SameLine();
	ImGui::Text("tick %d / %.3f | %.3fs | %.2f ticks/s",
		_debugFrameIndex,
		animation->GetFrameCount(),
		timeSeconds,
		ticksPerSecond);
	ImGui::Separator();

	const float listWidth = 330.0f;
	ImGui::BeginChild("BoneList", ImVec2(listWidth, 0.0f), true);
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::Separator();

	function<void(int)> DrawBoneTree = [&](int boneIndex)
		{
			shared_ptr<ModelBone> bone = mesh->GetBoneByIndex(boneIndex);
			const bool hasChildren = bone->childrenIndices.size() > 0;
			ImGuiTreeNodeFlags flags =
				ImGuiTreeNodeFlags_SpanAvailWidth |
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_OpenOnDoubleClick;
			if (_debugBoneIndex == boneIndex)
				flags |= ImGuiTreeNodeFlags_Selected;
			if (!hasChildren)
				flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

			const string boneName = Utils::ToString(bone->name);
			const bool open = ImGui::TreeNodeEx(
				(void*)(intptr_t)boneIndex, flags, "%s  [%d]", boneName.c_str(), boneIndex);
			if (ImGui::IsItemClicked())
				_debugBoneIndex = boneIndex;

			if (hasChildren && open)
			{
				for (int childIndex : bone->childrenIndices)
					DrawBoneTree(childIndex);
				ImGui::TreePop();
			}
		};

	for (int boneIndex = 0; boneIndex < mesh->GetBoneCount(); ++boneIndex)
	{
		if (mesh->GetBoneByIndex(boneIndex)->parentIndex < 0)
			DrawBoneTree(boneIndex);
	}
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginChild("BoneDetails", ImVec2(0.0f, 0.0f), true);

	{
		_debugBoneIndex = clamp(_debugBoneIndex, 0, static_cast<int>(mesh->GetBoneCount()) - 1);
        shared_ptr<ModelBone> curSelectBone = mesh->GetBoneByIndex(_debugBoneIndex);

		Matrix boneFinalMatrix = animTransforms[_debugAnimationIndex].transforms[_debugFrameIndex][_debugBoneIndex];
        Matrix boneGlobalMatrix = curSelectBone->offsetMatrix.Invert() * boneFinalMatrix;
        Matrix boneParentGlobalInverse = Matrix::Identity;
        if (curSelectBone->parentIndex >= 0)
            boneParentGlobalInverse = mesh->GetBoneByIndex(curSelectBone->parentIndex)->globalMatrix.Invert();
        Matrix boneLocalMatrix = boneGlobalMatrix * boneParentGlobalInverse;

		const string boneName = Utils::ToString(curSelectBone->name);
		const string parentName = curSelectBone->parentIndex >= 0
			? Utils::ToString(mesh->GetBoneByIndex(curSelectBone->parentIndex)->name)
			: "<root>";

		ImGui::Text("%s  [%d]", boneName.c_str(), _debugBoneIndex);
		ImGui::TextDisabled("Parent: %s [%d] | Children: %u",
			parentName.c_str(),
			curSelectBone->parentIndex,
			(uint32)curSelectBone->childrenIndices.size());
		ImGui::SeparatorText("Bone transforms");
		ImGui::TextDisabled("BoneLocal = Global * ParentGlobalInverse");
		ImGui::TextDisabled("Final = Offset * Global * GlobalInverseRoot");

		Vec3 position, eulerRotation, scale;
        Quaternion rotation;
		boneLocalMatrix.Decompose(scale, rotation, position);
		eulerRotation = Transform::ToEulerAngles(rotation);
        eulerRotation = MathUtils::RadToDeg(eulerRotation);

		OnGUIUtils::DrawVec3("Position", &position, 0.1f, true);
		OnGUIUtils::DrawVec3("Rotation", &eulerRotation, 0.1f, true);
		OnGUIUtils::DrawVec3("Scale", &scale, 0.1f, true);

		DrawDebugMatrix("Bone local matrix", boneLocalMatrix);
		DrawDebugMatrix("Global matrix", boneGlobalMatrix);
		DrawDebugMatrix("Bone offset matrix", curSelectBone->offsetMatrix);


		ImGui::SeparatorText("Model Socket SRT");
		vector<ModelSocket>& sockets = model->GetModelSockets();
		if (_debugSocketIndex >= static_cast<int>(sockets.size()))
			_debugSocketIndex = -1;

		string socketPreview = "None";
		if (_debugSocketIndex >= 0)
			socketPreview = to_string(_debugSocketIndex) + " : " + sockets[_debugSocketIndex].name;
		if (ImGui::BeginCombo("Socket", socketPreview.c_str()))
		{
			if (ImGui::Selectable("None", _debugSocketIndex < 0))
				_debugSocketIndex = -1;
			for (int socketIndex = 0; socketIndex < static_cast<int>(sockets.size()); ++socketIndex)
			{
				const ModelSocket& socket = sockets[socketIndex];
				string socketLabel = to_string(socketIndex) + " : " + socket.name;
				if (socket.boneIndex >= 0 && socket.boneIndex < static_cast<int32>(mesh->GetBoneCount()))
					socketLabel += " (" + Utils::ToString(mesh->GetBoneByIndex(socket.boneIndex)->name) + ")";
				if (ImGui::Selectable(socketLabel.c_str(), _debugSocketIndex == socketIndex))
					_debugSocketIndex = socketIndex;
			}
			ImGui::EndCombo();
		}

		if (_debugSocketIndex >= 0)
		{
			bool socketSrtChanged = false;
			ModelSocket& socket = sockets[_debugSocketIndex];
			if (ImGui::Button("Assign To Selected Bone"))
			{
				socket.boneIndex = _debugBoneIndex;
				socketSrtChanged = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset SRT"))
			{
				socket.localMatrix = Matrix::Identity;
				socketSrtChanged = true;
			}

			Vec3 socketPosition, socketScale, socketRotation;
			Quaternion socketQuaternion;
			socket.localMatrix.Decompose(socketScale, socketQuaternion, socketPosition);
			socketRotation = MathUtils::RadToDeg(Transform::ToEulerAngles(socketQuaternion));
			socketSrtChanged |= OnGUIUtils::DrawVec3("Socket Position", &socketPosition, 0.1f, false);
			socketSrtChanged |= OnGUIUtils::DrawVec3("Socket Rotation", &socketRotation, 0.1f, false);
			socketSrtChanged |= OnGUIUtils::DrawVec3("Socket Scale", &socketScale, 0.1f, false);
			if (socketSrtChanged)
			{
				const Vec3 radians = MathUtils::DegToRad(socketRotation);
				Matrix rotationMatrix = Matrix::CreateRotationX(radians.x);
				rotationMatrix *= Matrix::CreateRotationY(radians.y);
				rotationMatrix *= Matrix::CreateRotationZ(radians.z);
				socket.localMatrix = Matrix::CreateScale(socketScale) * rotationMatrix * Matrix::CreateTranslation(socketPosition);
				changed = true;
			}

			if (ImGui::Button("Save Socket"))
			{
				RESOURCES->SaveAsset(_model.GetAssetId());
			}

			if (socket.boneIndex >= 0 && socket.boneIndex < static_cast<int32>(mesh->GetBoneCount()))
			{
				const Matrix socketBoneFinal = animTransforms[_debugAnimationIndex].transforms[_debugFrameIndex][socket.boneIndex];
				const Matrix socketBoneGlobal = mesh->GetBoneByIndex(socket.boneIndex)->offsetMatrix.Invert() * socketBoneFinal;
				const Matrix socketWorld = socket.localMatrix * socketBoneGlobal * GetTransform()->GetWorldMatrix();
				EDITOR->GetSceneView()->SetTransformGizmoOverride(socketWorld,
					[this, model, socketIndex = _debugSocketIndex, socketBoneGlobal](const Matrix& worldMatrix)
					{
						vector<ModelSocket>& targetSockets = model->GetModelSockets();
						if (socketIndex < 0 || socketIndex >= static_cast<int>(targetSockets.size()))
							return;
						Matrix localMatrix = worldMatrix * GetTransform()->GetWorldMatrix().Invert() * socketBoneGlobal.Invert();
						targetSockets[socketIndex].localMatrix = localMatrix;
					});
			}
			else
			{
				ImGui::TextDisabled("Assign this socket to a valid bone to preview its gizmo.");
				EDITOR->GetSceneView()->ClearTransformGizmoOverride();
			}
		}
		else
		{
			EDITOR->GetSceneView()->SetTransformGizmoOverride(
				boneGlobalMatrix * GetTransform()->GetWorldMatrix(), [](const Matrix&) {});
		}
	}
	ImGui::EndChild();
	ImGui::End();

    if (wasOpen && _showAnimationDebug == false)
        EDITOR->GetSceneView()->ClearTransformGizmoOverride();

	return changed;
}

void ModelAnimator::DrawDebugMatrix(const char* label, const Matrix& matrix)
{
	if (!ImGui::TreeNode(label))
		return;

	if (ImGui::BeginTable("##Matrix", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame))
	{
		const float* values = &matrix._11;
		for (int row = 0; row < 4; ++row)
		{
			ImGui::TableNextRow();
			for (int column = 0; column < 4; ++column)
			{
				ImGui::TableSetColumnIndex(column);
				ImGui::Text("%.5f", values[row * 4 + column]);
			}
		}
		ImGui::EndTable();
	}
	ImGui::TreePop();
}

float ModelAnimator::GetNormalizedTime(const AnimationFrameDesc& animFrame)
{
    Model* model = _model.Resolve();
    if (model == nullptr)
        return 0.f;
    ModelAnimation* animation = model->GetAnimationByIndex(animFrame.animIndex);
    if (animation == nullptr || animation->GetFrameCount() <= 1)
        return 0.f;
    const uint32 modFrameCount = animation->GetFrameCount() - 1;
    const float framePosition = animFrame.curFrame + animFrame.ratio;
    return framePosition / modFrameCount;
}

float ModelAnimator::GetLeftTime(const AnimationFrameDesc& animFrame)
{
    Model* model = _model.Resolve();
    if (model == nullptr)
        return 0.f;
    ModelAnimation* animation = model->GetAnimationByIndex(animFrame.animIndex);
    if (animation == nullptr || animation->GetFrameCount() <= 1)
        return 0.f;

    const uint32 modFrameCount = animation->GetFrameCount() - 1;
    const float framePosition = animFrame.curFrame + animFrame.ratio;
    const float leftFrames = modFrameCount - framePosition;
    return leftFrames / animation->GetFrameRate();
}
