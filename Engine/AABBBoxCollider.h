#pragma once
#include "BaseCollider.h"
class AABBBoxCollider : public BaseCollider
{
    using Super = BaseCollider;
    DECLARE_COMPONENT(AABBBoxCollider)
public:
	AABBBoxCollider();
	virtual ~AABBBoxCollider();

	virtual void Update() override;
	virtual bool Intersects(Ray& ray, OUT float& distance) override;
	virtual bool Intersects(BaseCollider* other) override;

	BoundingBox& GetBoundingBox() { return _boundingBox; }

    virtual bool OnGUI() override;
    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_boundingBox.Center));
        ar(CEREAL_NVP(_boundingBox.Extents));
    }

private:
	BoundingBox _boundingBox;
};

