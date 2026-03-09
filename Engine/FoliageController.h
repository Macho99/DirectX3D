#pragma once
#include "Component.h"

class FoliageController : public Component
{
    using Super = Component;

public:
    static constexpr ComponentType StaticType = ComponentType::FoliageController;
    FoliageController();
    ~FoliageController();

    void Start() override;
    void SetBendFactor(float factor) { _foliageDesc.bendFactor = factor; }
    void SetStiffness(float stiffness) { _foliageDesc.stiffness = stiffness; }

    virtual bool OnGUI() override;

private:
    void BeforeRender(Material* material);
    FoliageDesc _foliageDesc;
    
public:
    static WindDesc S_WindDesc;
};

