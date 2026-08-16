#include "pch.h"
#include "InputText.h"
#include "InputManager.h"
#include "OnGUIUtils.h"

InputText::InputText() : Super(StaticType)
{
}

InputText::~InputText()
{
}

void InputText::Update()
{
    if (_isFocused && _containsMouseSelf == false)
    {
        if (INPUT->GetButtonDown(KEY_TYPE::LBUTTON) || INPUT->GetButtonDown(KEY_TYPE::RBUTTON))
        {
            SetFocused(false);
        }
    }

    if (_isFocused && _interactable)
        ProcessInput(INPUT->GetTextInputCharacters());

    UpdateCaret();
    Super::Update();
}

bool InputText::OnGUI()
{
    bool changed = Super::OnGUI();
    ImGui::Separator();

    if (OnGUIUtils::DrawBool("Interactable", &_interactable))
    {
        SetInteractable(_interactable);
        changed = true;
    }

    if (OnGUIUtils::DrawUInt32("Max Length", &_maxLength, 1.0f))
    {
        SetMaxLength(_maxLength);
        changed = true;
    }

    return changed;
}

void InputText::OnMouseDown()
{
    if (_interactable)
        SetFocused(true);
}

void InputText::OnDisable()
{
    SetFocused(false);
}

void InputText::SetFocused(bool focused)
{
    if (_isFocused == focused)
        return;

    _isFocused = focused;
    RestartCaretBlink();
}

void InputText::SetInteractable(bool interactable)
{
    _interactable = interactable;
    if (!_interactable)
        SetFocused(false);
}

void InputText::SetMaxLength(uint32 maxLength)
{
    _maxLength = maxLength;

    string text = GetText();
    TruncateUtf8(text, _maxLength);
    SetText(text);
}

void InputText::AddOnValueChangedEvent(function<void(const string&)> callback)
{
    _onValueChanged = std::move(callback);
}

void InputText::AddOnSubmitEvent(function<void(const string&)> callback)
{
    _onSubmit = std::move(callback);
}

void InputText::ProcessInput(const vector<uint32>& input)
{
    string text = GetText();
    size_t textLength = GetUtf8Length(text);
    bool changed = false;

    for (uint32 character : input)
    {
        if (character == '\b')
        {
            if (!text.empty())
            {
                PopUtf8Character(text);
                --textLength;
                changed = true;
            }
        }
        else if (character == '\r' || character == '\n')
        {
            if (_onSubmit)
                _onSubmit(text);
        }
        else if (character >= ' ' && textLength < _maxLength)
        {
            AppendUtf8(text, character);
            ++textLength;
            changed = true;
        }
    }

    if (changed)
    {
        SetText(text);
        RestartCaretBlink();
        if (_onValueChanged)
            _onValueChanged(text);
    }
}

void InputText::UpdateCaret()
{
    if (_isFocused && _interactable)
    {
        _caretBlinkTimer += DT;
        if (_caretBlinkTimer >= _caretBlinkInterval)
        {
            _caretBlinkTimer = std::fmod(_caretBlinkTimer, _caretBlinkInterval);
            _isCaretVisible = !_isCaretVisible;
            MarkTextDirty();
        }
    }
    else
    {
        _caretBlinkTimer = 0.0f;
        if (_isCaretVisible)
        {
            _isCaretVisible = false;
            MarkTextDirty();
        }
    }
}

void InputText::RestartCaretBlink()
{
    _caretBlinkTimer = 0.0f;
    const bool caretVisible = _isFocused && _interactable;
    if (_isCaretVisible != caretVisible)
    {
        _isCaretVisible = caretVisible;
        MarkTextDirty();
    }
}

void InputText::AppendUtf8(string& text, uint32 codepoint)
{
    if (codepoint <= 0x7F)
        text.push_back(static_cast<char>(codepoint));
    else if (codepoint <= 0x7FF)
    {
        text.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    else if (codepoint <= 0xFFFF)
    {
        text.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        text.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    else
    {
        text.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        text.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        text.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
}

size_t InputText::GetUtf8Length(const string& text)
{
    size_t length = 0;
    for (unsigned char character : text)
    {
        if ((character & 0xC0) != 0x80)
            ++length;
    }
    return length;
}

void InputText::PopUtf8Character(string& text)
{
    if (text.empty())
        return;

    size_t characterStart = text.size() - 1;
    while (characterStart > 0
        && (static_cast<unsigned char>(text[characterStart]) & 0xC0) == 0x80)
    {
        --characterStart;
    }

    text.erase(characterStart);
}

void InputText::TruncateUtf8(string& text, size_t maxLength)
{
    size_t length = 0;
    size_t index = 0;
    while (index < text.size() && length < maxLength)
    {
        ++index;
        while (index < text.size() && (static_cast<unsigned char>(text[index]) & 0xC0) == 0x80)
            ++index;
        ++length;
    }
    text.resize(index);
}
