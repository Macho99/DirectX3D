#pragma once
#include "Component.h"

class FoliageController : public Component
{
    using Super = Component;
    DECLARE_COMPONENT(FoliageController)
public:
    FoliageController();
    ~FoliageController();

    void Start() override;
    void SetBendFactor(float factor) { _foliageDesc.bendFactor = factor; }
    void SetStiffness(float stiffness) { _foliageDesc.stiffness = stiffness; }

    virtual bool OnGUI() override;
    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_foliageDesc.bendFactor), 
            CEREAL_NVP(_foliageDesc.stiffness));
    }

private:
    void BeforeRender(Material* material);
    FoliageDesc _foliageDesc;
    
public:
    static WindDesc S_WindDesc;
};

