#include "pch.h"
#include "Scene.h"
#include "GameObject.h"
#include "MonoBehaviour.h"
#include "BaseCollider.h"
#include "Camera.h"
#include "Terrain.h"
#include "Button.h"
#include "Sky.h"
#include "Transform.h"
#include "SlotManager.h"
#include "Component.h"
#include "Utils.h"
#include "Renderer.h"
#include "InstancingRenderer.h"
#include "InstancingBuffer.h"
#include "ModelRenderer.h"
#include "ModelAnimator.h"
#include "MathUtils.h"

Scene::Scene()
{
    _instanceId = Utils::GetRandomUInt64();
    Guid::SetCurrentInstanceId(_instanceId);
}

Scene::~Scene()
{

}

void Scene::Start()
{
    for (auto& transformRef : _rootObjects)
    {
        Transform* transform = transformRef.Resolve();
        ASSERT(transform != nullptr);
        GameObject* gameObject = transform->GetGameObject();
        ASSERT(gameObject != nullptr);

        gameObject->UpdateActiveInHierarchy(true, true);
        transform->UpdateTransform();
    }

    for (auto& gameObject : _gameObjects)
    {
        GameObject* obj = gameObject.Resolve();
        obj->Awake();
    }

    for (auto& gameObject : _gameObjects)
    {
        GameObject* obj = gameObject.Resolve();
        obj->Start();

        if (obj->IsActiveInHierarchy())
            obj->OnEnable();
    }

    for (auto& gameObject : _gameObjects)
    {
        GameObject* obj = gameObject.Resolve();
        Renderer* renderer = obj->GetRenderer();
        if (renderer == nullptr)
            continue;

        OnRendererAdd(renderer);
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
        if (obj->IsActiveInHierarchy())
            obj->OnDisable();
        obj->OnDestroy();
    }
    _gameObjects.clear();
    _removeLists.clear();

    _vecForward.clear();
    _vecBackward.clear();
    _instRenderers.clear();
}

void Scene::Update()
{
    for (auto& pair : _gameObjects)
    {
        GameObject* obj = pair.Resolve();
        if (obj->IsActiveInHierarchy())
            obj->Update();
    }
}

void Scene::LateUpdate()
{
    for (auto& pair : _gameObjects)
    {
        GameObject* obj = pair.Resolve();
        if (obj->IsActiveInHierarchy())
            obj->LateUpdate();
    }

    CheckCollision();
}

void Scene::Render()
{
	_renderCullingStats.fill(RenderCullingStats{});

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

            Render(_vecForward, cam, RenderTech::Shadow);
            RenderInstancing(cam, RenderTech::Shadow);
            //Viewport& vp = GRAPHICS->GetShadowViewport();
            Render(_vecBackward, cam, RenderTech::Shadow);
        }
    }

    memcpy(&Light::S_ShadowData.cascadeEnds, GRAPHICS->GetCascadeEnds(), sizeof(Light::S_ShadowData.cascadeEnds));
    Light::S_ShadowData.farLength = cam->GetFar();

    ////////////////////////////////////////////
    //				DrawNormalDepth
    ////////////////////////////////////////////
    GRAPHICS->ClearDepthStencilView();
    GRAPHICS->SetNormalDepthRenderTarget();

    Render(_vecForward, cam, RenderTech::NormalDepth);
    RenderInstancing(cam, RenderTech::NormalDepth);
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
    Render(_vecForward, cam, RenderTech::Draw);
    RenderInstancing(cam, RenderTech::Draw);
    if (_sky)
        _sky->Render(cam);
    Render(_vecBackward, cam, RenderTech::Draw);

    ////////////////////////////////////////////
    //				Distortion
    ////////////////////////////////////////////
    GRAPHICS->SetDistortionRenderTarget();
    Render(_vecForward, cam, RenderTech::Distortion);
    RenderInstancing(cam, RenderTech::Distortion);
    Render(_vecBackward, cam, RenderTech::Distortion);

    GRAPHICS->DrawPostProcesses();
}

void Scene::RenderUICamera(Camera* cam)
{
    GRAPHICS->ClearDepthStencilView();

    cam->SetStaticData();
    cam->SortUIGameObject();
    GRAPHICS->ClearDepthStencilView();
    const vector<ComponentRef<Renderer>>& uiRenderers = cam->GetUIRenderers();
    Render(uiRenderers, cam, RenderTech::Draw);
    //cam->Render_Backward(RenderTech::Draw);
}

GameObjectRef Scene::Add(string name, bool useRectTransform)
{
    GuidRef guidRef = _gameObjectSlotManager.CreateAndRegister<GameObject>(_instanceId, name);
    return Add(guidRef, useRectTransform, nullptr);
}

GameObjectRef Scene::Add(string name, Transform* parent)
{
    GuidRef guidRef = _gameObjectSlotManager.CreateAndRegister<GameObject>(_instanceId, name);
    RectTransform* rectParent = dynamic_cast<RectTransform*>(parent);
    return Add(guidRef, rectParent != nullptr, parent);
}

GameObjectRef Scene::Instantiate(const GameObjectRef& original, Transform* parent)
{
    GameObject* source = original.Resolve();
    if (source == nullptr || IsInScene(original) == false)
        return GameObjectRef();

    Transform* sourceTransform = source->GetTransform();
    const vector<TransformRef> sourceChildren = sourceTransform->GetChildren();
    const bool useRectTransform = dynamic_cast<RectTransform*>(sourceTransform) != nullptr;
    GameObjectRef targetRef = Add(source->GetName() + " (Clone)", useRectTransform);
    GameObject* target = targetRef.Resolve();
    ASSERT(target != nullptr);

    Transform* targetTransform = target->GetTransform();
    if (parent != nullptr)
        targetTransform->SetParent(parent);
    targetTransform->SetLocalScale(sourceTransform->GetLocalScale());
    targetTransform->SetLocalRotation(sourceTransform->GetLocalRotation());
    targetTransform->SetLocalPosition(sourceTransform->GetLocalPosition());

    const vector<ComponentDesc>& componentDescs = ComponentRegistry::Get().GetDescs();
    auto copyComponent = [&](Component* sourceComponent)
    {
        auto descIt = std::find_if(componentDescs.begin(), componentDescs.end(),
            [sourceComponent](const ComponentDesc& desc)
            {
                return desc.type == sourceComponent->GetType() && desc.matcher(sourceComponent);
            });
        if (descIt == componentDescs.end())
        {
            ASSERT(false);
            return;
        }

        unique_ptr<Component> targetComponentOwner = descIt->factory();
        Component* targetComponent = targetComponentOwner.get();
        target->AddComponent(std::move(targetComponentOwner));

        const Guid targetGuid = targetComponent->GetGuid();
        descIt->serializedCopier(sourceComponent, targetComponent);

        // Component::serialize also contains identity fields. The clone must retain
        // the identity allocated by AddComponent and only copy serialized state.
        targetComponent->SetGuid(targetGuid);
        targetComponent->SetGameObject(targetRef);

        if (target->IsActiveInHierarchy() && targetComponent->IsEnabled() == false)
            targetComponent->OnDisable();

        Renderer* targetRenderer = dynamic_cast<Renderer*>(targetComponent);
        if (targetRenderer != nullptr)
            OnRendererAdd(targetRenderer);
    };

    for (const ComponentRefBase& sourceComponentRef : source->GetAllFixedComponents())
    {
        Component* sourceComponent = sourceComponentRef.Resolve();
        if (sourceComponent == nullptr || sourceComponent->GetType() == ComponentType::Transform)
            continue;
        copyComponent(sourceComponent);
    }
    for (const ComponentRef<MonoBehaviour>& sourceScriptRef : source->GetScripts())
    {
        Component* sourceScript = sourceScriptRef.Resolve();
        if (sourceScript != nullptr)
            copyComponent(sourceScript);
    }

    target->SetLayerIndex(source->GetLayerIndex());
    target->SetActive(source->IsActiveInLocal());

    for (const TransformRef& sourceChildRef : sourceChildren)
    {
        Transform* sourceChild = sourceChildRef.Resolve();
        if (sourceChild != nullptr)
            Instantiate(sourceChild->GetGameObjectRef(), targetTransform);
    }

    return targetRef;
}

GuidRef Scene::AddComponent(GameObjectRef gameObjectRef, unique_ptr<Component> component)
{
    ComponentType type = component->GetType();
    GuidRef guidRef = GetComponentSlotManager()->RegisterExisting(std::move(component), _instanceId);
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
    GameObject* gameObject = gameObjectRef.Resolve();
    ASSERT(gameObject != nullptr);

    if (gameObject->IsActiveInHierarchy())
        gameObject->OnDisable();

    gameObject->OnDestroy();

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
    TransformRef transformRef = transform->GetRef();

    for (auto& child : transform->GetChildren())
        RemoveGameObjectRecur(child.Resolve()->GetGameObjectRef());

    if (transform->HasParent())
    {
        Transform* parent = transform->GetParent();
        vector<TransformRef>& siblings = parent->GetChildren();
        siblings.erase(std::remove(siblings.begin(), siblings.end(), transformRef), siblings.end());
    }
    else
    {
        _rootObjects.erase(std::remove(_rootObjects.begin(), _rootObjects.end(), transformRef), _rootObjects.end());
    }

    Renderer* renderer = gameObject->GetRenderer();
    if (renderer != nullptr)
        OnRendererRemove(renderer);

    _gameObjects.erase(gameObjectRef);
    _cameras.erase(gameObjectRef);
    _lights.erase(gameObjectRef);
    for (auto& componentRef : gameObject->GetAllFixedComponents())
    {
        _componentSlotManager.Remove(componentRef.guid);
    }
    for (auto& scriptRef : gameObject->GetScripts())
    {
        _componentSlotManager.Remove(scriptRef.guid);
    }
    _gameObjectSlotManager.Remove(gameObjectRef.guid);
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

GameObject* Scene::Pick(int32 screenX, int32 screenY)
{
    GameObject* camObj = GetMainCamera();
    if (camObj == nullptr)
        return nullptr;
    Camera* camera = camObj->GetCamera();

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

GameObjectRef Scene::Add(GuidRef guidRef, bool useRectTransform, Transform* parent)
{
    Guid::SetCurrentInstanceId(_instanceId);
    GameObjectRef gameObjectRef = GameObjectRef(guidRef);
    GameObject* gameObject = gameObjectRef.Resolve();

    if (useRectTransform)
    {
        gameObject->AddComponent(make_unique<RectTransform>());
        gameObject->SetLayerIndex(Layer_UI);
    }
    else
    {
        gameObject->AddComponent(make_unique<Transform>());
    }

    Transform* transform = gameObject->GetTransform();
    if (parent == nullptr)
    {
        _rootObjects.push_back(gameObject->GetFixedComponentRef<Transform>());
    }
    else
    {
        TransformRef parentRef = parent->GetRef();
        transform->SetParent(parentRef);
        Matrix parentMat = parent->GetWorldMatrix();
        transform->SetWorldMatrix(parentMat);
    }
    _gameObjects.insert(gameObjectRef);

    gameObject->Awake();
    gameObject->Start();
    gameObject->OnEnable();

    return gameObjectRef;
}

void Scene::OnRendererAdd(Renderer* renderer)
{
    ASSERT(renderer != nullptr);

    Material* mat = renderer->GetMaterial().Resolve();
    if (renderer->IsInstRenderer())
    {
        InstancingRenderer* instRenderer = static_cast<InstancingRenderer*>(renderer);
        OnInstRendererStateChange(ComponentRef<InstancingRenderer>(instRenderer),
            nullptr, mat, AssetId(), instRenderer->GetMeshId());
        return;
    }

    OnRendererMaterialChange(ComponentRef<Renderer>(renderer), nullptr, mat);
}

void Scene::OnRendererRemove(Renderer* renderer)
{
    ASSERT(renderer != nullptr);

    Material* mat = renderer->GetMaterial().Resolve();
    if (mat == nullptr)
        return;

    if (renderer->IsInstRenderer())
    {
        InstancingRenderer* instRenderer = static_cast<InstancingRenderer*>(renderer);
        OnInstRendererStateChange(ComponentRef<InstancingRenderer>(instRenderer),
            mat, nullptr, instRenderer->GetMeshId(), AssetId());
        return;
    }

    OnRendererMaterialChange(ComponentRef<Renderer>(renderer), mat, nullptr);
}

void Scene::OnInstRendererStateChange(ComponentRef<InstancingRenderer> instRendererRef, const Material* oldMat, const Material* newMat, const AssetId& oldMeshId, const AssetId& newMeshId)
{
    if (oldMat != nullptr && oldMeshId.IsValid())
    {
        auto materialIt = _instRenderers.find(oldMat->GetStringName());
        if (materialIt != _instRenderers.end())
        {
            auto meshIt = materialIt->second.find(oldMeshId);
            if (meshIt != materialIt->second.end())
            {
                auto& targetRenderers = meshIt->second.second;
                targetRenderers.erase(std::remove(targetRenderers.begin(), targetRenderers.end(), instRendererRef), targetRenderers.end());
                if (targetRenderers.empty())
                    materialIt->second.erase(meshIt);
            }
            if (materialIt->second.empty())
                _instRenderers.erase(materialIt);
        }
    }

    if (newMat != nullptr && newMeshId.IsValid())
    {
        vector<ComponentRef<InstancingRenderer>>& targetRenderers = _instRenderers[newMat->GetStringName()][newMeshId].second;
        if (std::find(targetRenderers.begin(), targetRenderers.end(), instRendererRef) == targetRenderers.end())
            targetRenderers.push_back(instRendererRef);
    }
}

void Scene::OnRendererMaterialChange(ComponentRef<Renderer> renderer, const Material* oldMat, const Material* newMat)
{
    if (oldMat != nullptr)
    {
        RenderQueue renderQueue = oldMat->GetRenderQueue();
        vector<ComponentRef<Renderer>>* vec = nullptr;
        switch (renderQueue)
        {
        case RenderQueue::Opaque:
        case RenderQueue::Cutout:
            vec = &_vecForward;
            break;
        case RenderQueue::Transparent:
            vec = &_vecBackward;
            break;
        }

        if (vec != nullptr)
            vec->erase(std::remove(vec->begin(), vec->end(), renderer), vec->end());
    }

    if (newMat != nullptr)
    {
        RenderQueue renderQueue = newMat->GetRenderQueue();
        switch (renderQueue)
        {
        case RenderQueue::Opaque:
        case RenderQueue::Cutout:
            if (std::find(_vecForward.begin(), _vecForward.end(), renderer) == _vecForward.end())
                _vecForward.push_back(renderer);
            break;
        case RenderQueue::Transparent:
            if (std::find(_vecBackward.begin(), _vecBackward.end(), renderer) == _vecBackward.end())
                _vecBackward.push_back(renderer);
            break;
        }
    }
}

void Scene::Render(const vector<ComponentRef<Renderer>>& renderers, Camera* camera, RenderTech renderTech)
{
    for (const ComponentRef<Renderer>& rendererRef : renderers)
    {
        Renderer* renderer = rendererRef.Resolve();
        ASSERT(renderer != nullptr);

        if (renderer->GetType() == ComponentType::SsrRenderer)
            continue;

        GameObject* gameObject = renderer->GetGameObject();
        if (camera->IsCulled(gameObject->GetLayerIndex()))
            continue;

        if (gameObject->IsActiveInHierarchy() == false)
            continue;

        if (renderer->TryInitialize() == false)
            continue;

        renderer->Render(renderTech);
    }
}

void Scene::RenderInstancing(Camera* camera, RenderTech renderTech)
{
	RenderCullingStats& cullingStats =
		_renderCullingStats[static_cast<size_t>(renderTech)];
    const Matrix cullingViewProjection = renderTech == RenderTech::Shadow
        ? Light::S_MatView * Light::S_MatProjection
        : camera->GetViewMatrix() * camera->GetProjectionMatrix();
    Vec4 frustumPlanes[6];
    MathUtils::ExtractFrustumPlanes(frustumPlanes, cullingViewProjection);

    for (auto& materialPair : _instRenderers)
    {
        const string& materialName = materialPair.first;
        auto& meshMap = materialPair.second;
        for (auto& meshPair : meshMap)
        {
            const AssetId& meshId = meshPair.first;
            InstancingBuffer& instBuffer = meshPair.second.first;

            instBuffer.ClearData();

            const vector<ComponentRef<InstancingRenderer>>& instRendererRefs = meshPair.second.second;
            InstancingRenderer* lastInstRenderer = nullptr;
            int tweenCount = 0;
            for (const ComponentRef<InstancingRenderer>& instRendererRef : instRendererRefs)
            {
                InstancingRenderer* instRenderer = instRendererRef.Resolve();
                GameObject* gameObject = instRenderer->GetGameObject();

                if (gameObject->IsActiveInHierarchy() == false)
                    continue;
                if (instRenderer->TryInitialize() == false)
                    continue;
				if (instRenderer->CanRender(renderTech) == false)
					continue;

				if (instRenderer->HasInstancingData() == false
					&& instRenderer->GetType() == ComponentType::ModelRenderer)
                {
					++cullingStats.totalCount;
					if (static_cast<ModelRenderer*>(instRenderer)->IsInFrustum(frustumPlanes) == false)
					{
						++cullingStats.culledCount;
						continue;
					}
                }
                else if (instRenderer->GetType() == ComponentType::ModelAnimator)
                {
                    const TweenDesc& tweenDesc = static_cast<ModelAnimator*>(instRenderer)->GetTweenDesc();
                    _instancedTweenDesc.tweens[tweenCount++] = tweenDesc;
                }

                if (instRenderer->HasInstancingData())
                {
                    instBuffer.AddData(instRenderer->GetInstancingDatas());
                }
                else
                {
                    Transform* transform = instRenderer->GetGameObject()->GetTransform();
                    instBuffer.AddData(transform->GetWorldMatrix());
                }

                lastInstRenderer = instRenderer;
            }

            if (lastInstRenderer != nullptr && instBuffer.GetCount() > 0)
            {
                if (tweenCount > 0)
                {
                    Material* material = lastInstRenderer->GetMaterial().Resolve();
                    Shader* shader = material != nullptr ? material->GetShader() : nullptr;
                    if (shader != nullptr)
                        shader->PushTweenData(_instancedTweenDesc);
                }

                lastInstRenderer->RenderInstancing(instBuffer, renderTech);
            }
        }
    }
}
