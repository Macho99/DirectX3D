#include "pch.h"
#include "FoliageController.h"
#include "Renderer.h"
#include "Material.h"

WindDesc FoliageController::S_WindDesc = WindDesc();

FoliageController::FoliageController() : Super(ComponentType::FoliageController)
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

void FoliageController::BeforeRender(Material* material)
{
    _foliageDesc.wind = S_WindDesc;
    _foliageDesc.time = TIME->GetGameTime();
    material->GetShader()->PushFoliageData(_foliageDesc);
}