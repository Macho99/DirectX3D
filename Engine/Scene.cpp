#include "pch.h"
#include "Scene.h"
#include "GameObject.h"
#include "BaseCollider.h"
#include "Camera.h"
#include "Terrain.h"
#include "Button.h"
#include "Sky.h"
#include "Transform.h"
#include "SlotManager.h"
#include "Component.h"
#include "Utils.h"

Scene::Scene()
{
    _gameObjectSlotManager = make_unique<SlotManager<GameObject>>();
    _componentSlotManager = make_unique<SlotManager<Component>>();
    _instanceId = Utils::GetRandomUInt64();
}

Scene::~Scene()
{

}

void Scene::Start()
{
	for (auto& pair : _gameObjects)
	{
        GameObject* obj = pair.Resolve();
		obj->Start();
	}
}

void Scene::OnDestroy()
{
	_cameras.clear();
    _lights.clear();
	_sky.reset();
    for (auto& pair : _gameObjects)
    {
		GameObject* obj = pair.Resolve();
        obj->OnDestroy();
    }
	_gameObjects.clear();
	_removeLists.clear();
}

void Scene::Update()
{
	for (auto& pair : _gameObjects)
	{
		GameObject* obj = pair.Resolve();
		obj->Update();
	}

	PickUI();
}

void Scene::LateUpdate()
{
	for (auto& pair : _gameObjects)
	{
		GameObject* obj = pair.Resolve();
		obj->LateUpdate();
	}

	CheckCollision();
}

void Scene::Render()
{
	for (auto& camera : _cameras)
	{
		Camera* cam = camera.Resolve()->GetCamera();
		if (cam->GetProjectionType() == ProjectionType::Perspective)
		{
			RenderGameCamera(cam);
		}
	}

	for (auto& camera : _cameras)
	{
		Camera* cam = camera.Resolve()->GetCamera();
		if (cam->GetProjectionType() != ProjectionType::Perspective)
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

	Light* light = GetLight()->GetLight();

	cam->SetStaticData();
	cam->SortGameObject();
	if (light)
	{
        Matrix VPinv = (cam->GetViewMatrix() * cam->GetProjectionMatrix()).Invert();

		for (int cascadeIdx = 0; cascadeIdx < NUM_SHADOW_CASCADES; cascadeIdx++)
		{
			GRAPHICS->ClearShadowDepthStencilView(cascadeIdx);
			GRAPHICS->SetShadowDepthStencilView(cascadeIdx);

			Vec3 frustumCornersWS[FRUSTUM_CORNERS];
			memcpy(frustumCornersWS, GRAPHICS->GetFrustumCornerNDC(), sizeof(frustumCornersWS));
			for (uint32 i = 0; i < 8; ++i)
				frustumCornersWS[i] = Vec3::Transform(frustumCornersWS[i], VPinv);

			// Unit Cube의 각 코너 위치를 Slice에 맞게 설정
			for (uint32 i = 0; i < 4; ++i)
			{
				Vec3 cornerRay = frustumCornersWS[i + 4] - frustumCornersWS[i];
				Vec3 nearCornerRay = cornerRay * GRAPHICS->GetCascadeEnd(cascadeIdx);
				Vec3 farCornerRay = cornerRay * GRAPHICS->GetCascadeEnd(cascadeIdx + 1);
				frustumCornersWS[i + 4] = frustumCornersWS[i] + farCornerRay;
				frustumCornersWS[i] = frustumCornersWS[i] + nearCornerRay;
			}

			// 뷰 프러스텀의 중심을 구함
			Vec3 frustumCenter(0.0f);
			for (uint32 i = 0; i < 8; ++i)
				frustumCenter += frustumCornersWS[i];
			frustumCenter *= (1.0f / 8.0f);

			// 뷰프러스텀의 바운드스피어의 반지름을 구함
			float sphereRadius = 0.0f;
			for (uint32 i = 0; i < 8; ++i)
			{
				float dist = (frustumCornersWS[i] - frustumCenter).Length();
				sphereRadius = max(sphereRadius, dist);
			}

			// 바운드 스피어의 반지름으로 AABB 정보 구성
			Vec3 mins(FLT_MAX);
			Vec3 maxes(-FLT_MAX);

			sphereRadius = std::ceil(sphereRadius * 16.0f) / 16.0f;
			maxes = Vec3(sphereRadius, sphereRadius, sphereRadius);
			mins = -maxes;

			// AABB의 크기를 구함
			Vec3 cascadeExtents = maxes - mins;

            Vec3 lightLook = light->GetTransform()->GetLook();
            Vec3 lightPos = frustumCenter - lightLook * fabs(mins.z);
			Matrix matView = ::XMMatrixLookAtLH(lightPos, lightPos + lightLook, Vec3::Up);
			light->SetVPMatrix(matView, ::XMMatrixOrthographicLH(cascadeExtents.x, cascadeExtents.y, 0, cascadeExtents.z), cascadeIdx);

			cam->Render_Forward(RenderTech::Shadow);
			//Viewport& vp = GRAPHICS->GetShadowViewport();
			cam->Render_Backward(RenderTech::Shadow);
		}
	}

	memcpy(&Light::S_ShadowData.cascadeEnds, GRAPHICS->GetCascadeEnds(), sizeof(Light::S_ShadowData.cascadeEnds));
    Light::S_ShadowData.farLength = cam->GetFar();

	////////////////////////////////////////////
	//				DrawNormalDepth
	////////////////////////////////////////////
	GRAPHICS->ClearDepthStencilView();
	GRAPHICS->SetNormalDepthRenderTarget();

	cam->Render_Forward(RenderTech::NormalDepth);
	//cam->Render_Backward(RenderTech::NormalDepth);

	////////////////////////////////////////////
	//					Ssao
	////////////////////////////////////////////
	GRAPHICS->DrawSsaoMap(INPUT->GetButton(KEY_TYPE::LCTRL));

	////////////////////////////////////////////
	//					Draw
	////////////////////////////////////////////
	GRAPHICS->ClearDepthStencilView();
	GRAPHICS->SetRTVAndDSV();
	cam->Render_Forward(RenderTech::Draw);
	if (_sky)
		_sky->Render(cam);
	cam->Render_Backward(RenderTech::Draw);
	GRAPHICS->DrawPostProcesses();
}

void Scene::RenderUICamera(Camera* cam)
{
	GRAPHICS->ClearDepthStencilView();

	cam->SetStaticData();
	cam->SortGameObject();
	cam->Render_Forward(RenderTech::Draw);
	cam->Render_Backward(RenderTech::Draw);
}

GameObjectRef Scene::Add(unique_ptr<GameObject> gameObjectUnique)
{
	GuidRef guidRef = _gameObjectSlotManager->RegisterExisting(std::move(gameObjectUnique));
	return Add(guidRef);
}

GameObjectRef Scene::Add(string name)
{
    auto gameObjectUnique = make_unique<GameObject>(name);
    return Add(std::move(gameObjectUnique));
}

GuidRef Scene::AddComponent(GameObjectRef gameObjectRef, unique_ptr<Component> component)
{
	ComponentType type = component->GetType();
	GuidRef guidRef = GetComponentSlotManager()->RegisterExisting(std::move(component));
	if (type == ComponentType::Camera)
	{
		_cameras.insert(gameObjectRef);
	}
	else if (type == ComponentType::Light)
	{
		_lights.insert(gameObjectRef);
	}
	return guidRef;
}

void Scene::Remove(GameObjectRef gameObjectRef)
{
	_removeLists.push_back(gameObjectRef);
}

void Scene::CleanUpRemoveLists()
{
	for (const GameObjectRef& gameObject : _removeLists)
	{
		RemoveGameObjectRecur(gameObject);
	}
	_removeLists.clear();
}

void Scene::RemoveGameObjectRecur(const GameObjectRef& gameObjectRef)
{
    GameObject* gameObject = gameObjectRef.Resolve();
	Transform* transform = gameObject->GetTransform();

	// 2) 자식 먼저 전부 삭제
	for (auto& child : transform->GetChildren())
		RemoveGameObjectRecur(child.Resolve()->GetGameObjectRef());

	if (transform->HasParent() == false)
	{
		_rootObjects.erase(std::remove(_rootObjects.begin(), _rootObjects.end(), TransformRef(transform->GetGuid())), _rootObjects.end());
	}

	_gameObjects.erase(gameObjectRef);
	_cameras.erase(gameObjectRef);
	_lights.erase(gameObjectRef);
}

GameObject* Scene::GetMainCamera()
{
	for (auto& cameraRef : _cameras)
	{
        GameObject* camera = cameraRef.Resolve();
		if (camera->GetCamera()->GetProjectionType() == ProjectionType::Perspective)
			return camera;
	}
	return nullptr;
}

GameObject* Scene::GetUICamera()
{
	for (auto& cameraRef : _cameras)
	{
		GameObject* camera = cameraRef.Resolve();
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

	Camera* camera = GetUICamera()->GetCamera();
	
	for (auto& gameObjectRef : _gameObjects)
	{
		GameObject* gameObject = gameObjectRef.Resolve();
		if (gameObject->GetButton() == nullptr)
			continue;

		if (gameObject->GetButton()->Picked(screenPt))
			gameObject->GetButton()->InvokeOnClicked();
	}
}

GameObject* Scene::Pick(int32 screenX, int32 screenY)
{
	Camera* camera = GetMainCamera()->GetCamera();

    const GameDesc& gameDesc = GAME->GetGameDesc();
	float width = gameDesc.sceneWidth;
	float height = gameDesc.sceneHeight;
	screenX -= gameDesc.scenePos.x;
	screenY -= gameDesc.scenePos.y;

	Matrix projectionMatrix = camera->GetProjectionMatrix();

	float viewX = (2.0f * screenX / width - 1.0f) / projectionMatrix(0, 0);
	float viewY = (-2.0f * screenY / height + 1.0f) / projectionMatrix(1, 1);

	Matrix viewMatrixInv = camera->GetViewMatrix().Invert();

	float minDistance = FLT_MAX;
	GameObject* picked = nullptr;

	// ViewSpace에서 Ray 정의
	Vec4 rayOrigin = Vec4(0.f, 0.f, 0.f, 1.f);
	Vec4 rayDir = Vec4(viewX, viewY, 1.f, 0.f);

	Vec3 worldRayOrigin = XMVector3TransformCoord(rayOrigin, viewMatrixInv);
	Vec3 worldRayDir = XMVector3TransformNormal(rayDir, viewMatrixInv);
	worldRayDir.Normalize();

	// WorldSpace에서 연산
	Ray ray = Ray(worldRayOrigin, worldRayDir);

	for (auto& gameObjectRef : _gameObjects)
	{
        GameObject* gameObject = gameObjectRef.Resolve();
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

	for (auto& gameObjectRef : _gameObjects)
	{
		GameObject* gameObject = gameObjectRef.Resolve();
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
	vector<BaseCollider*> colliders;
	
	for (auto& gameObjectRef : _gameObjects)
	{
        GameObject* object = gameObjectRef.Resolve();
		if (object->GetCollider() == nullptr)
			continue;

		colliders.push_back(object->GetCollider());
	}

	// BruteForce
	for (int32 i = 0; i < colliders.size(); i++)
	{
		for (int32 j = i + 1; j < colliders.size(); j++)
		{
			BaseCollider* col1 = colliders[i];
			BaseCollider* col2 = colliders[j];
			if (col1->Intersects(col2))
			{

			}

		}
	}
}

GameObjectRef Scene::Add(GuidRef guidRef)
{
    Guid::SetCurrentInstanceId(_instanceId);
	GameObjectRef gameObjectRef = GameObjectRef(guidRef);
	GameObject* gameObject = gameObjectRef.Resolve();

	Transform* transform = gameObject->GetTransform();
	if (transform->HasParent() == false)
	{
		_rootObjects.push_back(TransformRef(transform->GetGuid()));
	}
	_gameObjects.insert(gameObjectRef);
	return gameObjectRef;
}