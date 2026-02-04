#include "pch.h"
#include "InputManager.h"

void InputManager::Init(HWND hwnd)
{
	_hwnd = hwnd;
	_states.resize(KEY_TYPE_COUNT, KEY_STATE::NONE);
}

void InputManager::Update()
{
	HWND hwnd = ::GetActiveWindow();
	if (_hwnd != hwnd)
	{
		for (uint32 key = 0; key < KEY_TYPE_COUNT; key++)
			_states[key] = KEY_STATE::NONE;

		return;
	}

    GameDesc& gameDesc = GAME->GetGameDesc();
	if (gameDesc.isEditor && gameDesc.sceneFocused == false)
	{
		for (uint32 key = 0; key < KEY_TYPE_COUNT; key++)
		{
            if (_states[key] == KEY_STATE::PRESS || _states[key] == KEY_STATE::DOWN)
                _states[key] = KEY_STATE::UP;
            else
				_states[key] = KEY_STATE::NONE;
		}
        return;
	}

	BYTE asciiKeys[KEY_TYPE_COUNT] = {};
	if (::GetKeyboardState(asciiKeys) == false)
		return;

	for (uint32 key = 0; key < KEY_TYPE_COUNT; key++)
	{
		// 키가 눌려 있으면 true
		if (asciiKeys[key] & 0x80)
		{
			KEY_STATE& state = _states[key];

			// 이전 프레임에 키를 누른 상태라면 PRESS
			if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN)
				state = KEY_STATE::PRESS;
			else
				state = KEY_STATE::DOWN;
		}
		else
		{
			KEY_STATE& state = _states[key];

			// 이전 프레임에 키를 누른 상태라면 UP
			if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN)
				state = KEY_STATE::UP;
			else
				state = KEY_STATE::NONE;
		}
	}

	::GetCursorPos(&_mousePos);
	::ScreenToClient(_hwnd, &_mousePos);

	if (gameDesc.isEditor)
	{
		//DBG->Log(std::format("mousePose: {} {}", _mousePos.x, _mousePos.y));
		Vec2 scenePos = gameDesc.scenePos;
		float width = gameDesc.sceneWidth;
		float height = gameDesc.sceneHeight;

		bool inScene = false;
		if (scenePos.x <= _mousePos.x && _mousePos.x <= scenePos.x + width)
		{
			if (scenePos.y <= _mousePos.y && _mousePos.y <= scenePos.y + height)
				inScene = true;
		}

		//DBG->Log("inScene: " + string(inScene ? "true" : "false"));
		if (inScene == false)
		{
			if (GetButtonDown(KEY_TYPE::LBUTTON))
				SetState(KEY_TYPE::LBUTTON, KEY_STATE::NONE);
			if (GetButtonDown(KEY_TYPE::RBUTTON))
				SetState(KEY_TYPE::RBUTTON, KEY_STATE::NONE);
		}
	}
}