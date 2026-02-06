#pragma once
#include "BaseCollider.h"
class SphereCollider : public BaseCollider
{
	using Super = BaseCollider;
public:
	SphereCollider();
	virtual ~SphereCollider();

	virtual void Update() override;
	virtual bool Intersects(Ray& ray, OUT float& distance) override;
	virtual bool Intersects(BaseCollider* other) override;

	void SetRadius(float radius) { _radius = radius; }
	BoundingSphere& GetBoundingSphere() { return _boundingSphere; }

private:
	float _radius = 1.f;
	BoundingSphere _boundingSphere;
};

