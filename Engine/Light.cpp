#include "pch.h"
#include "Light.h"

Light::Light() : Super(ComponentType::Light)
{

}

Light::~Light()
{
}

void Light::Update()
{
	//RENDER->PushLightData(_desc);
}
