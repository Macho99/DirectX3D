#pragma once
#include "TargetFollower.h"

class ModelAnimator;

class ModelSocketFollower : public TargetFollower
{
    using Super = TargetFollower;
    DECLARE_COMPONENT(ModelSocketFollower)

public:
    ModelSocketFollower();
    ~ModelSocketFollower();

    void SetModelAnimator(ModelAnimator* animator);
    ModelAnimator* GetModelAnimator() const;

    void SetSocketName(const string& socketName) { _socketName = socketName; }
    const string& GetSocketName() const { return _socketName; }

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(
            CEREAL_NVP(_modelAnimator),
            CEREAL_NVP(_socketName)
        );
    }

protected:
    virtual bool TryGetTargetPose(OUT Vec3& position, OUT Vec3& rotation) override;
    virtual bool DrawTargetGUI() override;

private:
    ComponentRef<ModelAnimator> _modelAnimator;
    string _socketName;
};
