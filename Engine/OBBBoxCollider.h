#pragma once
#include "BaseCollider.h"

class OBBBoxCollider : public BaseCollider
{
    using Super = BaseCollider;
public:
    static constexpr ComponentType StaticType = ComponentType::OBBBoxCollider;
	OBBBoxCollider();
	virtual ~OBBBoxCollider();

	virtual void Update() override;
	virtual bool Intersects(Ray& ray, OUT float& distance) override;
	virtual bool Intersects(BaseCollider* other) override;

	BoundingOrientedBox& GetBoundingBox() { return _boundingBox; }

	virtual bool OnGUI() override;
	template<typename Archive>
	void serialize(Archive& ar)
	{
        Super::serialize(ar);
		ar(
            CEREAL_NVP(_boundingBox.Center),
            CEREAL_NVP(_boundingBox.Extents),
            CEREAL_NVP(_boundingBox.Orientation)
			);
	}

private:
	BoundingOrientedBox _boundingBox;
};