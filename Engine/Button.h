#pragma once
#include "UIImage.h"

class Button : public UIImage
{
    using Super = UIImage;
    DECLARE_COMPONENT(Button)
public:
	Button();
	virtual ~Button();

	void AddOnClickedEvent(std::function<void(void)> func);
	void InvokeOnClicked();
	void SetInteractable(bool interactable);
	bool IsInteractable() const { return _interactable; }

	virtual void OnMouseDown() override;
	virtual void OnMouseUp() override;
	virtual void OnDisable() override;
	virtual bool OnGUI() override;
    virtual int GetVersion() const override { return 1; }

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        if (Archive::is_saving::value || _version >= 1)
            ar(CEREAL_NVP(_interactable));
    }

private:
	std::function<void(void)> _onClicked;
	bool _interactable = true;
	bool _isPressed = false;
};

