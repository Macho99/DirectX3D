#include "pch.h"
#include "InputManager.h"
#include "Scene.h"
#include "SlotManager.h"
#include "UIRenderer.h"

void InputManager::Init(HWND hwnd)
{
    _hwnd = hwnd;
    _states.resize(KEY_TYPE_COUNT, KEY_STATE::NONE);

    RAWINPUTDEVICE mouseDevice = {};
    mouseDevice.usUsagePage = 0x01;
    mouseDevice.usUsage = 0x02;
    mouseDevice.hwndTarget = hwnd;
    ::RegisterRawInputDevices(&mouseDevice, 1, sizeof(mouseDevice));
}

void InputManager::AddTextInputCharacter(wchar_t character)
{
    if (0xD800 <= character && character <= 0xDBFF)
    {
        _pendingHighSurrogate = character;
        return;
    }

    if (0xDC00 <= character && character <= 0xDFFF && _pendingHighSurrogate != 0)
    {
        const uint32 high = static_cast<uint32>(_pendingHighSurrogate - 0xD800);
        const uint32 low = static_cast<uint32>(character - 0xDC00);
        _pendingTextInput.push_back(0x10000 + (high << 10) + low);
        _pendingHighSurrogate = 0;
        return;
    }

    if (0xDC00 <= character && character <= 0xDFFF)
        return;

    _pendingHighSurrogate = 0;
    _pendingTextInput.push_back(static_cast<uint32>(character));
}

void InputManager::Update()
{
    _textInput.swap(_pendingTextInput);
    _pendingTextInput.clear();
    _mouseInScene = false;
    _prevMouseWheelDelta = _curMouseWheelDelta;
    _curMouseWheelDelta = 0;
    _mouseDelta = _isMouseCaptured ? _pendingMouseDelta : POINT();
    _pendingMouseDelta = POINT();

    HWND hwnd = ::GetActiveWindow();
    if (_hwnd != hwnd)
    {
        ShowMouseCursor();
        for (uint32 key = 0; key < KEY_TYPE_COUNT; key++)
            _states[key] = KEY_STATE::NONE;

        ClearUIInput();
        return;
    }

    GameDesc& gameDesc = GAME->GetGameDesc();
    if (gameDesc.isEditor && gameDesc.sceneFocused == false)
    {
        ShowMouseCursor();
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

    if (_isMouseCaptured)
    {
        if (GetButtonDown(KEY_TYPE::ESC))
            ShowMouseCursor();

        const LONG centerX = static_cast<LONG>(gameDesc.sceneWidth * 0.5f);
        const LONG centerY = static_cast<LONG>(gameDesc.sceneHeight * 0.5f);

        _mousePos.x = centerX;
        _mousePos.y = centerY;
        _mouseInScene = true;
        CenterMouseCursor();
        return;
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
    bool curMousePressed = GetButton(KEY_TYPE::LBUTTON) || GetButtonDown(KEY_TYPE::LBUTTON);
    bool prevMousePressed = _mousePressed;
    _mousePressed = curMousePressed;

    UIRenderer* prevPicked = _pickedUIRef.Resolve();
    UIRenderer* curPicked;
    if (curMousePressed && prevMousePressed)
        curPicked = _pickedUIRef.Resolve();
    else
        curPicked = PickUI();
    _pickedUIRef = UIRendererRef(curPicked);

    if (curMousePressed && !prevMousePressed)
    {
        _dragHandlerRef = UIRendererRef();
        _isDragging = false;
    }

    POINT prevMousePressedPos = _mousePressedPos;
    POINT curMousePressedPos = curMousePressed ? _mousePos : POINT();
    _mousePressedPos = curMousePressedPos;
    _isMouseOnUI = (curPicked != nullptr);

    if (curPicked == prevPicked)
    {
        if (curPicked == nullptr)
            return;

        if (curMousePressed)
        {
            if (prevMousePressed)
            {
                Vec2 delta = Vec2(static_cast<float>(curMousePressedPos.x - prevMousePressedPos.x), static_cast<float>(curMousePressedPos.y - prevMousePressedPos.y));
                UIRenderer* dragHandlerRenderer = _dragHandlerRef.Resolve();

                if (dragHandlerRenderer == nullptr)
                {
                    dragHandlerRenderer = FindDragHandler(curPicked);
                    _dragHandlerRef = UIRendererRef(dragHandlerRenderer);
                }
                IDragHandler* dragHandler = dynamic_cast<IDragHandler*>(dragHandlerRenderer);

                if (dragHandler != nullptr)
                {
                    if (!_isDragging && (delta.LengthSquared() == 0))
                        return;

                    if (!_isDragging)
                    {
                        dragHandler->OnBeginDrag();
                        _isDragging = true;
                    }
                    dragHandler->OnDrag(DragEvent{ delta });
                }
            }
            else
                curPicked->OnMouseDown();
        }
        else
        {
            if (prevMousePressed)
            {
                IDragHandler* dragHandler = dynamic_cast<IDragHandler*>(_dragHandlerRef.Resolve());
                if (_isDragging && dragHandler != nullptr)
                    dragHandler->OnEndDrag();

                _dragHandlerRef = UIRendererRef();
                _isDragging = false;
                curPicked->OnMouseUp();
            }
            else
                curPicked->OnMouseStay();
        }
    }
    else
    {
        if (prevPicked != nullptr)
        {
            if (prevMousePressed)
            {
                IDragHandler* dragHandler = dynamic_cast<IDragHandler*>(_dragHandlerRef.Resolve());
                if (_isDragging && dragHandler != nullptr)
                    dragHandler->OnEndDrag();

                _dragHandlerRef = UIRendererRef();
                _isDragging = false;
                prevPicked->OnMouseUp();
            }
            prevPicked->OnMouseExit();
        }
        if (curPicked != nullptr)
        {
            curPicked->OnMouseEnter();
            if (curMousePressed)
                curPicked->OnMouseDown();
        }
    }
}

void InputManager::CaptureMouseCursor()
{
    if (_isMouseCaptured)
        return;

    _isMouseCaptured = true;
    _mouseDelta = POINT();
    _pendingMouseDelta = POINT();
    while (::ShowCursor(FALSE) >= 0)
    {
    }
    CenterMouseCursor();
}

void InputManager::ShowMouseCursor()
{
    if (!_isMouseCaptured)
        return;

    _isMouseCaptured = false;
    _mouseDelta = POINT();
    _pendingMouseDelta = POINT();
    while (::ShowCursor(TRUE) < 0)
    {
    }
}

void InputManager::AddRawMouseDelta(LONG x, LONG y)
{
    if (!_isMouseCaptured)
        return;

    _pendingMouseDelta.x += x;
    _pendingMouseDelta.y += y;
}

void InputManager::CenterMouseCursor()
{
    const GameDesc& gameDesc = GAME->GetGameDesc();
    POINT center = {
        static_cast<LONG>(gameDesc.sceneWidth * 0.5f),
        static_cast<LONG>(gameDesc.sceneHeight * 0.5f)
    };

    if (gameDesc.isEditor)
    {
        center.x += static_cast<LONG>(gameDesc.scenePos.x);
        center.y += static_cast<LONG>(gameDesc.scenePos.y);
    }

    ::ClientToScreen(_hwnd, &center);
    ::SetCursorPos(center.x, center.y);
}

void InputManager::ClearUIInput()
{
    UIRenderer* pickedUI = _pickedUIRef.Resolve();
    IDragHandler* dragHandler = dynamic_cast<IDragHandler*>(_dragHandlerRef.Resolve());
    if (_isDragging && dragHandler != nullptr)
        dragHandler->OnEndDrag();

    if (pickedUI != nullptr)
    {
        if (_mousePressed)
            pickedUI->OnMouseUp();
        pickedUI->OnMouseExit();
    }

    _pickedUIRef = UIRendererRef();
    _dragHandlerRef = UIRendererRef();
    _isDragging = false;
    _mousePressed = false;
    _mousePressedPos = POINT();
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

        UIRenderer* pickedUI = PickUIFromTransform(rootTransform, nullptr);
        if (pickedUI != nullptr)
            return pickedUI;
    }

    return nullptr;
}

UIRenderer* InputManager::PickUIFromTransform(Transform* transform, UIRenderer* maskRenderer)
{
    GameObject* gameObject = transform->GetGameObject();
    if (gameObject == nullptr || !gameObject->IsActiveInHierarchy())
        return nullptr;

    Renderer* renderer = gameObject->GetRenderer();
    UIRenderer* uiRenderer = dynamic_cast<UIRenderer*>(renderer);
    if (uiRenderer != nullptr && uiRenderer->IsEnabled() == false)
        uiRenderer = nullptr;
    UIMaskMode maskMode = UIMaskMode::None;
    if (uiRenderer != nullptr)
    {
        maskMode = uiRenderer->GetMaskMode();
        if (maskMode == UIMaskMode::VisibleMask || maskMode == UIMaskMode::InvisibleMask)
            maskRenderer = uiRenderer;
    }

    vector<TransformRef>& children = transform->GetChildren();
    for (auto it = children.rbegin(); it != children.rend(); ++it)
    {
        Transform* childTransform = it->Resolve();
        if (childTransform == nullptr)
            continue;

        GameObject* childGameObject = childTransform->GetGameObject();
        if (childGameObject == nullptr || !childGameObject->IsActiveInHierarchy())
            continue;

        UIRenderer* pickedUI = PickUIFromTransform(childTransform, maskRenderer);
        if (pickedUI != nullptr)
            return pickedUI;
    }

    if (uiRenderer != nullptr)
    {
        if (maskMode == UIMaskMode::MaskTarget)
        {
            if (maskRenderer == nullptr)
            {
                return nullptr;
            }
            else
            {
                bool canPick = maskRenderer->ContainsMouseSelf() && uiRenderer->ContainsMouseSelf();
                if (canPick)
                    return uiRenderer;

                return nullptr;
            }
        }
        else if (maskMode == UIMaskMode::InvisibleMask)
            return nullptr;
        else
        {
            if (uiRenderer->ContainsMouseSelf())
                return uiRenderer;
        }
    }

    return nullptr;
}

UIRenderer* InputManager::FindDragHandler(UIRenderer* uiRenderer)
{
    if (uiRenderer == nullptr)
        return nullptr;

    Transform* transform = uiRenderer->GetTransform();
    while (transform != nullptr)
    {
        GameObject* gameObject = transform->GetGameObject();
        if (gameObject != nullptr && gameObject->IsActiveInHierarchy())
        {
            Renderer* renderer = gameObject->GetRenderer();
            UIRenderer* candidate = dynamic_cast<UIRenderer*>(renderer);
            if (candidate != nullptr && candidate->IsEnabled() && dynamic_cast<IDragHandler*>(candidate) != nullptr)
                return candidate;
        }

        transform = transform->GetParent();
    }

    return nullptr;
}
