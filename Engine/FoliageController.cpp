#include "pch.h"
#include "FoliageController.h"
#include "Renderer.h"
#include "Material.h"
#include "OnGUIUtils.h"

WindDesc FoliageController::S_WindDesc = WindDesc();

FoliageController::FoliageController() : Super(StaticType)
{
}

FoliageController::~FoliageController()
{
}

void FoliageController::Start()
{
    // [this]를 사용하여 현재 FoliageController 객체를 캡처
    auto callback = [this](Material* material)
        {
            this->BeforeRender(material);
        };
    GetGameObject()->GetRenderer()->SetBeforeRender(callback);
}

bool FoliageController::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
    changed |= OnGUIUtils::DrawFloat("Time", &_foliageDesc.time, 1.f, true);
    changed |= OnGUIUtils::DrawFloat("Bend Factor", &_foliageDesc.bendFactor, 0.01f, false);
    changed |= OnGUIUtils::DrawFloat("Stiffness", &_foliageDesc.stiffness, 0.01f, false);

    ImGui::Separator();
    changed |= OnGUIUtils::DrawVec3("Wind Direction", &S_WindDesc.windDirection, 0.01f, false);
    changed |= OnGUIUtils::DrawFloat("Wind Strength", &S_WindDesc.windStrength, 0.01f, false);
    changed |= OnGUIUtils::DrawFloat("Wave Frequency", &S_WindDesc.waveFrequency, 0.01f, false);

    return changed;
}

void FoliageController::BeforeRender(Material* material)
{
    _foliageDesc.wind = S_WindDesc;
    _foliageDesc.time = TIME->GetGameTime();
    material->GetShader()->PushFoliageData(_foliageDesc);
}