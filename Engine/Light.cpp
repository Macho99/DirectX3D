#include "pch.h"
#include "Light.h"
#include "Camera.h"
#include "OnGUIUtils.h"

Matrix Light::S_MatView = Matrix::Identity;
Matrix Light::S_MatProjection = Matrix::Identity;
ShadowDesc Light::S_ShadowData = {};

Light::Light() : Super(StaticType)
{

}

Light::~Light()
{
}

void Light::Update()
{
    _desc.direction = GetTransform()->GetLook();
}

bool Light::OnGUI()
{
	LightDesc& lightDesc = GetLightDesc();
    bool changed = false;
	changed |= OnGUIUtils::DrawColor("Ambient", &lightDesc.ambient, false);
	changed |= OnGUIUtils::DrawColor("Diffuse", &lightDesc.diffuse, false);
	changed |= OnGUIUtils::DrawColor("Specular", &lightDesc.specular, false);
	changed |= OnGUIUtils::DrawColor("Emissive", &lightDesc.emissive, false);

    return changed;
}

void Light::SetVPMatrix(Matrix matView, Matrix matProjection, int cascadeIdx)
{
	S_MatView = matView;
	S_MatProjection = matProjection;

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	Matrix T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	S_ShadowData.Transforms[cascadeIdx] = S_MatView * S_MatProjection * T;
}
