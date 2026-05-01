#include "pch.h";
#include "ComponentRegistry.h"
#include "AABBBoxCollider.h"
#include "Billboard.h"
#include "Button.h"
#include "Camera.h"
#include "FoliageController.h"
#include "GrassRenderer.h"
#include "Light.h"
#include "MeshRenderer.h"
#include "ModelAnimator.h"
#include "ModelRenderer.h"
#include "OBBBoxCollider.h"
#include "ParticleSystem.h"
#include "SnowBillboard.h"
#include "SphereCollider.h"
#include "TessTerrain.h"
#include "Terrain.h"
#include "Transform.h"
#include "NavMesh.h"
#include "MonoBehaviour.h"
#include "LineRenderer.h"
#include "NavAgent.h"

static void ForceRegisterEngineComponents()
{
    AABBBoxCollider::EnsureAutoRegister();
    Billboard::EnsureAutoRegister();
    Button::EnsureAutoRegister();
    Camera::EnsureAutoRegister();
    FoliageController::EnsureAutoRegister();
    GrassRenderer::EnsureAutoRegister();
    Light::EnsureAutoRegister();
    MeshRenderer::EnsureAutoRegister();
    ModelAnimator::EnsureAutoRegister();
    ModelRenderer::EnsureAutoRegister();
    OBBBoxCollider::EnsureAutoRegister();
    ParticleSystem::EnsureAutoRegister();
    SnowBillboard::EnsureAutoRegister();
    SphereCollider::EnsureAutoRegister();
    TessTerrain::EnsureAutoRegister();
    Terrain::EnsureAutoRegister();
    Transform::EnsureAutoRegister();
    NavMesh::EnsureAutoRegister();
    LineRenderer::EnsureAutoRegister();
    NavAgent::EnsureAutoRegister();
}

void ComponentRegistry::Init()
{
    ForceRegisterEngineComponents();

    sort(_descs.begin(), _descs.end(), [](const ComponentDesc& a, const ComponentDesc& b)
        {
            return string(a.name) < string(b.name);
        });
}


CEREAL_REGISTER_TYPE(Transform);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, Transform);

CEREAL_REGISTER_TYPE(MeshRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, MeshRenderer);

CEREAL_REGISTER_TYPE(ModelRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ModelRenderer);

CEREAL_REGISTER_TYPE(ModelAnimator);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ModelAnimator);

CEREAL_REGISTER_TYPE(Camera);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, Camera);

CEREAL_REGISTER_TYPE(Light);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, Light);

CEREAL_REGISTER_TYPE(AABBBoxCollider);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, AABBBoxCollider);

CEREAL_REGISTER_TYPE(OBBBoxCollider);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, OBBBoxCollider);

CEREAL_REGISTER_TYPE(SphereCollider);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, SphereCollider);

CEREAL_REGISTER_TYPE(Terrain);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, Terrain);

CEREAL_REGISTER_TYPE(TessTerrain);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, TessTerrain);

CEREAL_REGISTER_TYPE(Button);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, Button);

CEREAL_REGISTER_TYPE(Billboard);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, Billboard);
// 아직 직렬화 안함

CEREAL_REGISTER_TYPE(SnowBillboard);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, SnowBillboard);

CEREAL_REGISTER_TYPE(ParticleSystem);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ParticleSystem);
// 직렬화 미완

CEREAL_REGISTER_TYPE(FoliageController);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, FoliageController);
// 직렬화 미완

CEREAL_REGISTER_TYPE(GrassRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, GrassRenderer);

//CEREAL_REGISTER_TYPE(MonoBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, MonoBehaviour);

CEREAL_REGISTER_TYPE(NavMesh);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, NavMesh);

CEREAL_REGISTER_TYPE(LineRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, LineRenderer);

CEREAL_REGISTER_TYPE(NavAgent);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, NavAgent);