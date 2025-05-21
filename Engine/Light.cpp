#include "pch.h"
#include "Light.h"
#include "Camera.h"

Matrix Light::S_MatView = Matrix::Identity;
Matrix Light::S_MatProjection = Matrix::Identity;

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

void Light::SetVPMatrix(Camera* camera, float backDist, Matrix matProjection)
{
	Vec3 camPos = camera->GetTransform()->GetPosition();
	Vec3 lookVec = GetTransform()->GetLook();

	Vec3 eyePosition = camPos - lookVec * backDist;
	Vec3 focusPosition = eyePosition + lookVec;
	Vec3 upDirection = Vec3::Up;
	S_MatView = ::XMMatrixLookAtLH(eyePosition, focusPosition, upDirection);
	S_MatProjection = matProjection;
}
