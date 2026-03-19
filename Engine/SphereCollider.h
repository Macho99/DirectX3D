#pragma once
#include "BaseCollider.h"
class SphereCollider : public BaseCollider
{
	using Super = BaseCollider;
    DECLARE_COMPONENT(SphereCollider)
public:
	SphereCollider();
	virtual ~SphereCollider();

	virtual void Update() override;
	virtual bool Intersects(Ray& ray, OUT float& distance) override;
	virtual bool Intersects(BaseCollider* other) override;

	void SetRadius(float radius) { _radius = radius; }
	BoundingSphere& GetBoundingSphere() { return _boundingSphere; }

    virtual bool OnGUI() override;
    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(
            CEREAL_NVP(_radius),
            CEREAL_NVP(_boundingSphere.Center)
            //CEREAL_NVP(_boundingSphere.Radius)) radius is calculated from scale, so no need to serialize
            );
    }

private:
	float _radius = 1.f;
	BoundingSphere _boundingSphere;
};

