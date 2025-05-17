#include "pch.h"
#include "Camera.h"
#include "Scene.h"
#include "Renderer.h"
#include "Material.h"

Matrix Camera::S_MatView = Matrix::Identity;
Matrix Camera::S_MatProjection = Matrix::Identity;

Camera::Camera() : Super(ComponentType::Camera)
{
	_width = static_cast<float>(GAME->GetGameDesc().width);
	_height = static_cast<float>(GAME->GetGameDesc().height);
}

Camera::~Camera()
{

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

void Camera::SortGameObject()
{
	shared_ptr<Scene> scene = CUR_SCENE;
	unordered_set<shared_ptr<GameObject>>& gameObjects = scene->GetObjects();

	_vecForward.clear();
	_vecBackward.clear();

	for (auto& gameObject : gameObjects)
	{
		if (IsCulled(gameObject->GetLayerIndex()))
			continue;

		shared_ptr<Renderer> renderer = gameObject->GetRenderer();
		if (renderer == nullptr)
			continue;

		shared_ptr<Material> material = renderer->GetMaterial();
		RenderQueue renderQueue = material->GetRenderQueue();

		// TODO: 컷아웃용 정렬하기
		// TODO: 거리에 따라 정렬하기(인스턴싱도 고민)

		switch (renderQueue)
		{
		case RenderQueue::Opaque:
			_vecForward.push_back(gameObject);
			break;
		case RenderQueue::Transparent:
			_vecBackward.push_back(gameObject);
			break;
		}
	}
}

void Camera::Render_Forward()
{
	S_MatView = _matView;
	S_MatProjection = _matProjection;

	GET_SINGLE(RenderManager)->Render(_vecForward);
}

void Camera::Render_Backward()
{
	GET_SINGLE(RenderManager)->Render(_vecBackward);
}
