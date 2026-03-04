#include "pch.h"
#include "Light.h"
#include "Camera.h"

Matrix Light::S_MatView = Matrix::Identity;
Matrix Light::S_MatProjection = Matrix::Identity;
ShadowDesc Light::S_ShadowData = {};

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

void Light::OnGUI()
{
    bool showAlpha = true; // 알파 채널 표시 여부

    ImGuiColorEditFlags flags = 0;
    flags |= ImGuiColorEditFlags_DisplayRGB;        // RGB 표시
    flags |= ImGuiColorEditFlags_InputRGB;          // RGB 입력
    flags |= ImGuiColorEditFlags_Float;             // 0~1 float
    //flags |= ImGuiColorEditFlags_HDR;
    if (!showAlpha)
        flags |= ImGuiColorEditFlags_NoAlpha;


    LightDesc& lightDesc = GetLightDesc();
    ImGui::ColorEdit4("Ambient", &lightDesc.ambient.x, flags);
    ImGui::ColorEdit4("Diffuse", &lightDesc.diffuse.x, flags);
    ImGui::ColorEdit4("Specular", &lightDesc.specular.x, flags);
    ImGui::ColorEdit4("Emissive", &lightDesc.emissive.x, flags);
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
