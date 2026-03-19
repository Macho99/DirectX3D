#pragma once
#include "Component.h"
class Button : public Component
{
    using Super = Component;
    DECLARE_COMPONENT(Button)
public:
	Button();
	virtual ~Button();

	bool Picked(POINT screenPos);

	void Create(Vec2 screenPos, Vec2 size, ResourceRef<Material> material);

	void AddOnClickedEvent(std::function<void(void)> func);
	void InvokeOnClicked();

	virtual bool OnGUI() override;
    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_rect));
    }

private:
	std::function<void(void)> _onClicked;
	RECT _rect;
};

