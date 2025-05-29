#include "pch.h"
#include "Scene.h"
#include "GameObject.h"
#include "BaseCollider.h"
#include "Camera.h"
#include "Terrain.h"
#include "Button.h"
#include "Sky.h"

void Scene::Start()
{
	auto copys = _gameObjects;
	for (shared_ptr<GameObject> obj : copys)
	{
		obj->Start();
	}
}

void Scene::Update()
{
	auto copys = _gameObjects;
	for (shared_ptr<GameObject> obj : copys)
	{
		obj->Update();
	}

	PickUI();
}

void Scene::LateUpdate()
{
	auto copys = _gameObjects;
	for (shared_ptr<GameObject> obj : copys)
	{
		obj->LateUpdate();
	}

	CheckCollision();
}

void Scene::Render()
{
	for (auto& camera : _cameras)
	{
		Camera* cam = camera->GetCamera().get();
		if (cam->GetProjectionType() == ProjectionType::Perspective)
		{
			RenderGameCamera(cam);
		}
		else
		{
			RenderUICamera(cam);
		}

	}
}

void Scene::RenderGameCamera(Camera* cam)
{
	////////////////////////////////////////////
	//				DrawShadow
	////////////////////////////////////////////
	GRAPHICS->ClearShadowDepthStencilView();
	GRAPHICS->SetShadowDepthStencilView();

	Light* light = GetLight()->GetLight().get();

	cam->SetStaticData();
	cam->SortGameObject();
	if (light)
	{
		light->SetVPMatrix(cam, 100.0f, ::XMMatrixOrthographicLH(100, 100, 0, 200));

		cam->Render_Forward(RenderTech::Shadow);
		//Viewport& vp = GRAPHICS->GetShadowViewport();
		cam->Render_Backward(RenderTech::Shadow);
	}


	////////////////////////////////////////////
	//				DrawNormalDepth
	////////////////////////////////////////////
	GRAPHICS->ClearDepthStencilView();
	GRAPHICS->SetNormalDepthRenderTarget();

	cam->Render_Forward(RenderTech::NormalDepth);
	//cam->Render_Backward(RenderTech::NormalDepth);

	////////////////////////////////////////////
	//					Draw
	////////////////////////////////////////////
	GRAPHICS->ClearDepthStencilView();
	GRAPHICS->SetRTVAndDSV();
	cam->Render_Forward(RenderTech::Draw);
	if (_sky)
		_sky->Render(cam);
	cam->Render_Backward(RenderTech::Draw);
}

void Scene::RenderUICamera(Camera* cam)
{
	GRAPHICS->ClearDepthStencilView();

	cam->SetStaticData();
	cam->SortGameObject();
	cam->Render_Forward(RenderTech::Draw);
	cam->Render_Backward(RenderTech::Draw);
}

void Scene::Add(shared_ptr<GameObject> gameObject)
{
	_gameObjects.insert(gameObject);
	if (gameObject->GetFixedComponent(ComponentType::Camera) != nullptr)
	{
		_cameras.insert(gameObject);
	}
	if (gameObject->GetFixedComponent(ComponentType::Light) != nullptr)
	{
		_lights.insert(gameObject);
	}
}

void Scene::Remove(shared_ptr<GameObject> gameObject)
{
	_gameObjects.erase(gameObject);
	_cameras.erase(gameObject);
	_lights.erase(gameObject);
}

shared_ptr<GameObject> Scene::GetMainCamera()
{
	for (auto& camera : _cameras)
	{
		if (camera->GetCamera()->GetProjectionType() == ProjectionType::Perspective)
			return camera;
	}
	return nullptr;
}

shared_ptr<GameObject> Scene::GetUICamera()
{
	for (auto& camera : _cameras)
	{
		if (camera->GetCamera()->GetProjectionType() == ProjectionType::Orthographic)
			return camera;
	}
	return nullptr;
}

void Scene::PickUI()
{
	if (INPUT->GetButtonDown(KEY_TYPE::LBUTTON) == false)
		return;

	if (GetUICamera() == nullptr)
		return;

	POINT screenPt = INPUT->GetMousePos();

	shared_ptr<Camera> camera = GetUICamera()->GetCamera();
	
	const auto gameObjects = GetObjects();
	
	for (auto& gameObject : gameObjects)
	{
		if (gameObject->GetButton() == nullptr)
			continue;

		if (gameObject->GetButton()->Picked(screenPt))
			gameObject->GetButton()->InvokeOnClicked();
	}
}

shared_ptr<GameObject> Scene::Pick(int32 screenX, int32 screenY)
{
	shared_ptr<Camera> camera = GetMainCamera()->GetCamera();

	float width = GRAPHICS->GetViewport().GetWidth();
	float height = GRAPHICS->GetViewport().GetHeight();

	Matrix projectionMatrix = camera->GetProjectionMatrix();

	float viewX = (2.0f * screenX / width - 1.0f) / projectionMatrix(0, 0);
	float viewY = (-2.0f * screenY / height + 1.0f) / projectionMatrix(1, 1);

	Matrix viewMatrixInv = camera->GetViewMatrix().Invert();

	float minDistance = FLT_MAX;
	shared_ptr<GameObject> picked;

	// ViewSpace에서 Ray 정의
	Vec4 rayOrigin = Vec4(0.f, 0.f, 0.f, 1.f);
	Vec4 rayDir = Vec4(viewX, viewY, 1.f, 0.f);

	Vec3 worldRayOrigin = XMVector3TransformCoord(rayOrigin, viewMatrixInv);
	Vec3 worldRayDir = XMVector3TransformNormal(rayDir, viewMatrixInv);
	worldRayDir.Normalize();

	// WorldSpace에서 연산
	Ray ray = Ray(worldRayOrigin, worldRayDir);

	for (auto& gameObject : _gameObjects)
	{
		if (camera->IsCulled(gameObject->GetLayerIndex()))
			continue;

		if (gameObject->GetCollider() == nullptr)
			continue;

		float distance = 0.f;
		if (gameObject->GetCollider()->Intersects(ray, OUT distance) == false)
			continue;

		if (distance < minDistance)
		{
			minDistance = distance;
			picked = gameObject;
		}
	}

	for (auto& gameObject : _gameObjects)
	{
		if (gameObject->GetTerrain() == nullptr)
			continue;

		Vec3 pickPos;
		float distance = 0.0f;
		if (gameObject->GetTerrain()->Pick(screenX, screenY, OUT pickPos, OUT distance) == false)
			continue;

		if (distance < minDistance)
		{
			minDistance = distance;
			picked = gameObject;
		}
	}

	return picked;
}

void Scene::CheckCollision()
{
	vector<shared_ptr<BaseCollider>> colliders;
	
	for (shared_ptr<GameObject> object : _gameObjects)
	{
		if (object->GetCollider() == nullptr)
			continue;

		colliders.push_back(object->GetCollider());
	}

	// BruteForce
	for (int32 i = 0; i < colliders.size(); i++)
	{
		for (int32 j = i + 1; j < colliders.size(); j++)
		{
			shared_ptr<BaseCollider>& col1 = colliders[i];
			shared_ptr<BaseCollider>& col2 = colliders[j];
			if (col1->Intersects(col2))
			{

			}

		}
	}
}
