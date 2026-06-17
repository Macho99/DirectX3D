#include "pch.h"
#include "InputManager.h"
#include "Scene.h"
#include "SlotManager.h"
#include "UIRenderer.h"

void InputManager::Init(HWND hwnd)
{
    _hwnd = hwnd;
    _states.resize(KEY_TYPE_COUNT, KEY_STATE::NONE);
}

void InputManager::Update()
{
    _mouseInScene = false;
    _prevMouseWheelDelta = _curMouseWheelDelta;
    _curMouseWheelDelta = 0;

    HWND hwnd = ::GetActiveWindow();
    if (_hwnd != hwnd)
    {
        for (uint32 key = 0; key < KEY_TYPE_COUNT; key++)
            _states[key] = KEY_STATE::NONE;

        ClearUIInput();
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

        ClearUIInput();
        return;
    }

    BYTE asciiKeys[KEY_TYPE_COUNT] = {};
    if (::GetKeyboardState(asciiKeys) == false)
        return;

    for (uint32 key = 0; key < KEY_TYPE_COUNT; key++)
    {
        if (asciiKeys[key] & 0x80)
        {
            KEY_STATE& state = _states[key];

            if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN)
                state = KEY_STATE::PRESS;
            else
                state = KEY_STATE::DOWN;
        }
        else
        {
            KEY_STATE& state = _states[key];

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
        Vec2 scenePos = gameDesc.scenePos;
        _mousePos.x -= static_cast<LONG>(scenePos.x);
        _mousePos.y -= static_cast<LONG>(scenePos.y);
    }

    float width = gameDesc.sceneWidth;
    float height = gameDesc.sceneHeight;

    if (0 <= _mousePos.x && _mousePos.x <= width)
    {
        if (0 <= _mousePos.y && _mousePos.y <= height)
            _mouseInScene = true;
    }

    if (_mouseInScene == false)
    {
        if (GetButtonDown(KEY_TYPE::LBUTTON))
            SetState(KEY_TYPE::LBUTTON, KEY_STATE::NONE);
        if (GetButtonDown(KEY_TYPE::RBUTTON))
            SetState(KEY_TYPE::RBUTTON, KEY_STATE::NONE);
    }

    UpdateUIInput();
}

void InputManager::UpdateUIInput()
{
    UIRenderer* prevHoveredUI = _hoveredUIRef.Resolve();
    UIRenderer* curHoveredUI = _mouseInScene ? PickUI() : nullptr;

    if (prevHoveredUI != curHoveredUI)
    {
        if (prevHoveredUI != nullptr)
            prevHoveredUI->OnMouseExit();

        if (curHoveredUI != nullptr)
        {
            _hoveredUIRef = UIRendererRef(curHoveredUI);
            curHoveredUI->OnMouseEnter();
        }
        else
        {
            _hoveredUIRef = UIRendererRef();
        }
    }

    if (curHoveredUI != nullptr)
        curHoveredUI->OnMouseStay();

    if (curHoveredUI != nullptr && GetButtonDown(KEY_TYPE::LBUTTON))
    {
        _pressedUIRef = UIRendererRef(curHoveredUI);
        curHoveredUI->OnMouseDown();
    }

    if (GetButtonUp(KEY_TYPE::LBUTTON))
    {
        UIRenderer* pressedUI = _pressedUIRef.Resolve();
        if (pressedUI != nullptr)
        {
            pressedUI->OnMouseUp();

            if (pressedUI == curHoveredUI)
                pressedUI->OnMouseClick();
        }

        _pressedUIRef = UIRendererRef();
    }
}

void InputManager::ClearUIInput()
{
    UIRenderer* hoveredUI = _hoveredUIRef.Resolve();
    if (hoveredUI != nullptr)
        hoveredUI->OnMouseExit();

    UIRenderer* pressedUI = _pressedUIRef.Resolve();
    if (pressedUI != nullptr && pressedUI != hoveredUI)
        pressedUI->OnMouseUp();

    _hoveredUIRef = UIRendererRef();
    _pressedUIRef = UIRendererRef();
}

UIRenderer* InputManager::PickUI()
{
    auto scene = CUR_SCENE;
    if (scene == nullptr)
        return nullptr;

    vector<TransformRef>& rootObjects = scene->GetRootObjects();
    for (auto it = rootObjects.rbegin(); it != rootObjects.rend(); ++it)
    {
        Transform* rootTransform = it->Resolve();
        if (rootTransform == nullptr)
            continue;

        GameObject* rootGameObject = rootTransform->GetGameObject();
        if (rootGameObject == nullptr || !rootGameObject->IsActiveInHierarchy())
            continue;

        UIRenderer* pickedUI = PickUIFromTransform(rootTransform);
        if (pickedUI != nullptr)
            return pickedUI;
    }

    return nullptr;
}

UIRenderer* InputManager::PickUIFromTransform(Transform* transform)
{
    vector<TransformRef>& children = transform->GetChildren();
    for (auto it = children.rbegin(); it != children.rend(); ++it)
    {
        Transform* childTransform = it->Resolve();
        if (childTransform == nullptr)
            continue;

        GameObject* childGameObject = childTransform->GetGameObject();
        if (childGameObject == nullptr || !childGameObject->IsActiveInHierarchy())
            continue;

        UIRenderer* pickedUI = PickUIFromTransform(childTransform);
        if (pickedUI != nullptr)
            return pickedUI;
    }

    GameObject* gameObject = transform->GetGameObject();
    if (gameObject == nullptr || !gameObject->IsActiveInHierarchy())
        return nullptr;

    Renderer* renderer = gameObject->GetRenderer();
    UIRenderer* uiRenderer = dynamic_cast<UIRenderer*>(renderer);
    if (uiRenderer != nullptr && uiRenderer->ContainsMouseSelf())
        return uiRenderer;

    return nullptr;
}