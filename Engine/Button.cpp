#include "pch.h"
#include "Button.h"
#include "OnGUIUtils.h"

Button::Button()
	:Super(StaticType)
{
}

Button::~Button()
{
}

void Button::AddOnClickedEvent(std::function<void(void)> func)
{
	_onClicked = func;
}

void Button::InvokeOnClicked()
{
	if (_interactable && _onClicked)
		_onClicked();
}

void Button::SetInteractable(bool interactable)
{
	_interactable = interactable;
	if (!_interactable)
		_isPressed = false;
}

void Button::OnMouseDown()
{
	if (_interactable)
		_isPressed = true;
}

void Button::OnMouseUp()
{
	const bool shouldInvoke = _interactable && _isPressed && ContainsMouseSelf();
	_isPressed = false;

	if (shouldInvoke)
		InvokeOnClicked();
}

void Button::OnDisable()
{
	_isPressed = false;
}

bool Button::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
    ImGui::Separator();
    if (OnGUIUtils::DrawBool("Interactable", &_interactable))
    {
		SetInteractable(_interactable);
		changed = true;
	}
    return changed;
}
