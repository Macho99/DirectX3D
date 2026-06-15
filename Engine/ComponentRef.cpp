#include "pch.h"
#include "ComponentRef.h"

#include "Scene.h"         // GetComponentSlotManager, Scene 정의
#include "SlotManager.h"   // SlotManager<T>
#include "Component.h"     // Component

#include "Transform.h"
#include "Camera.h"
#include "Light.h"
#include "TessTerrain.h"
#include "MonoBehaviour.h"
#include "ModelAnimator.h"
#include "MeshRenderer.h"
#include "LineRenderer.h"
#include "LayoutGroup.h"
#include "VerticalLayoutGroup.h"
#include "HorizontalLayoutGroup.h"
#include "GridLayoutGroup.h"
#include "NavMesh.h"
#include "NavAgent.h"
#include "SsrRenderer.h"
#include "Text.h"
#include "UIImage.h"
#include "RectTransform.h"

template<class T>
T* ComponentRef<T>::Resolve() const
{
    if (!IsValid())
        return nullptr;

    SlotManager<Component>* manager = CUR_SCENE->GetComponentSlotManager();
    uint64 sceneInstanceId = CUR_SCENE->GetInstanceId();

    if (!cached.IsValid() || cachedSceneInstanceId != sceneInstanceId)
    {
        cached = manager->FindHandle(guid);
        cachedSceneInstanceId = sceneInstanceId;
    }

    Component* p = manager->Resolve(cached);

#ifdef _DEBUG
    return dynamic_cast<T*>(p);
#else
    return static_cast<T*>(p);
#endif
}

// 명시적 인스턴스화
template struct ComponentRef<Component>;
template struct ComponentRef<Transform>;
template struct ComponentRef<Camera>;
template struct ComponentRef<Light>;
template struct ComponentRef<TessTerrain>;
template struct ComponentRef<MonoBehaviour>;
template struct ComponentRef<ModelAnimator>;
template struct ComponentRef<MeshRenderer>;
template struct ComponentRef<LineRenderer>;
template struct ComponentRef<VerticalLayoutGroup>;
template struct ComponentRef<HorizontalLayoutGroup>;
template struct ComponentRef<GridLayoutGroup>;
template struct ComponentRef<NavMesh>;
template struct ComponentRef<NavAgent>;
template struct ComponentRef<SsrRenderer>;
template struct ComponentRef<Text>;
template struct ComponentRef<UIImage>;
template struct ComponentRef<RectTransform>;

