#pragma once
#include "BaseCollider.h"

class OBBBoxCollider : public BaseCollider
{
public:
    static constexpr ComponentType StaticType = ComponentType::OBBBoxCollider;
	OBBBoxCollider();
	virtual ~OBBBoxCollider();

	virtual void Update() override;
	virtual bool Intersects(Ray& ray, OUT float& distance) override;
	virtual bool Intersects(BaseCollider* other) override;

	BoundingOrientedBox& GetBoundingBox() { return _boundingBox; }

private:
	BoundingOrientedBox _boundingBox;
};

