#include "pch.h"
#include "Camera.h"
#include "Scene.h"
#include "Renderer.h"
#include "Material.h"
#include "OnGUIUtils.h"

Matrix Camera::S_MatView = Matrix::Identity;
Matrix Camera::S_MatProjection = Matrix::Identity;
Vec3 Camera::S_Pos = Vec3::Zero;

Camera::Camera() : Super(ComponentType::Camera)
{
    OnSize();
}

Camera::~Camera()
{

}

void Camera::OnSize()
{
	_width = static_cast<float>(GAME->GetGameDesc().sceneWidth);
	_height = static_cast<float>(GAME->GetGameDesc().sceneHeight);
}

void Camera::LateUpdate()
{
	UpdateMatrix();
}

void Camera::UpdateMatrix()
{
	Vec3 eyePosition = GetTransform()->GetPosition();
	Vec3 focusPosition = eyePosition + GetTransform()->GetLook();
	Vec3 upDirection = GetTransform()->GetUp();
	_matView = ::XMMatrixLookAtLH(eyePosition, focusPosition, upDirection);

	if (_type == ProjectionType::Perspective)
	{
		_matProjection = ::XMMatrixPerspectiveFovLH(_fov, _width / _height, _near, _far);
	}
	else
	{
		_matProjection = ::XMMatrixOrthographicLH(_width, _height, _near, _far);
	}
}

bool Camera::OnGUI()
{
	bool changed = false;

	changed |= Super::OnGUI();
    changed |= OnGUIUtils::DrawEnumCombo("Projection Type", _type, ProjectionTypeNames, (int)ProjectionType::Max);
    changed |= OnGUIUtils::DrawFloat("Near", &_near, 0.1f);
    changed |= OnGUIUtils::DrawFloat("Far", &_far, 0.1f);
    changed |= OnGUIUtils::DrawFloat("FOV", &_fov, 0.01f);
    changed |= OnGUIUtils::DrawFloat("Width", &_width, 1.f);
    changed |= OnGUIUtils::DrawFloat("Height", &_height, 1.f);
    changed |= OnGUIUtils::DrawUInt32("Culling Mask", &_cullingMask, 1.f);

	return changed;
}

void Camera::SortGameObject()
{
	shared_ptr<Scene> scene = CUR_SCENE;
	GameObjectRefSet& gameObjects = scene->GetObjects();

	_vecForward.clear();
	_vecBackward.clear();

	for (auto& gameObjectRef : gameObjects)
	{
		GameObject* gameObject = gameObjectRef.Resolve();
		if (IsCulled(gameObject->GetLayerIndex()))
			continue;

		Renderer* renderer = gameObject->GetRenderer();
		if (renderer == nullptr)
			continue;

		Material* material = renderer->GetMaterial().Resolve();
        if (material == nullptr)
            continue;

		RenderQueue renderQueue = material->GetRenderQueue();

		// TODO: 컷아웃용 정렬하기
		// TODO: 거리에 따라 정렬하기(인스턴싱도 고민)

		switch (renderQueue)
		{
		case RenderQueue::Opaque:
		case RenderQueue::Cutout:
			_vecForward.push_back(gameObject);
			break;
		case RenderQueue::Transparent:
			_vecBackward.push_back(gameObject);
			break;
		}
	}
}

void Camera::SetStaticData()
{
	S_MatView = _matView;
	S_MatProjection = _matProjection;
	S_Pos = GetTransform()->GetPosition();
}

void Camera::Render_Forward(RenderTech renderTech)
{
	GET_SINGLE(RenderManager)->Render(_vecForward, renderTech);
}

void Camera::Render_Backward(RenderTech renderTech)
{
	GET_SINGLE(RenderManager)->Render(_vecBackward, renderTech);
}
