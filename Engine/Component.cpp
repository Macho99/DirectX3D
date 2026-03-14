#include "pch.h"
#include "Component.h"
#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "ModelRenderer.h"
#include "ModelAnimator.h"
#include "Camera.h"
#include "Light.h"
#include "AABBBoxCollider.h"
#include "OBBBoxCollider.h"
#include "SphereCollider.h"
#include "Terrain.h"
#include "TessTerrain.h"
#include "Button.h"
#include "Billboard.h"
#include "SnowBillboard.h"
#include "ParticleSystem.h"
#include "FoliageController.h"
#include "GrassRenderer.h"
#include "MonoBehaviour.h"

Component::Component() : _type(ComponentType::End)
{
}

Component::Component(ComponentType type) : _type(type)
{

}

Component::~Component()
{

}

GameObject* Component::GetGameObject()
{
	return _gameObject.Resolve();
}

Transform* Component::GetTransform()
{
	return _gameObject.Resolve()->GetTransform();
}

bool Component::OnGUI()
{
	return false;
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