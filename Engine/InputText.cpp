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
    Super::Update();

    if (_isFocused && _containsMouseSelf == false)
    {
        if (INPUT->GetButtonDown(KEY_TYPE::LBUTTON) || INPUT->GetButtonDown(KEY_TYPE::RBUTTON))
        {
            SetFocused(false);
        }
    }

    if (_isFocused && _interactable)
        ProcessInput(INPUT->GetTextInputCharacters());
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
    _isFocused = focused;
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

void InputText::ProcessInput(const wstring& input)
{
    string text = GetText();
    size_t textLength = GetUtf8Length(text);
    bool changed = false;

    for (size_t i = 0; i < input.size(); ++i)
    {
        const wchar_t character = input[i];
        if (character == L'\b')
        {
            if (!text.empty())
            {
                PopUtf8Character(text);
                --textLength;
                changed = true;
            }
        }
        else if (character == L'\r' || character == L'\n')
        {
            if (_onSubmit)
                _onSubmit(text);
        }
        else if (character >= L' ' && textLength < _maxLength)
        {
            wstring utf16(1, character);
            if (0xD800 <= character && character <= 0xDBFF && i + 1 < input.size())
            {
                const wchar_t lowSurrogate = input[i + 1];
                if (0xDC00 <= lowSurrogate && lowSurrogate <= 0xDFFF)
                {
                    utf16.push_back(lowSurrogate);
                    ++i;
                }
            }

            const string utf8 = Utf16ToUtf8(utf16);
            if (!utf8.empty())
            {
                text += utf8;
                ++textLength;
                changed = true;
            }
        }
    }

    if (changed)
    {
        SetText(text);
        if (_onValueChanged)
            _onValueChanged(text);
    }
}

string InputText::Utf16ToUtf8(const wstring& text)
{
    if (text.empty())
        return string();

    const int size = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0)
        return string();

    string result(size, '\0');
    ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
    return result;
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

    text.pop_back();
    while (!text.empty() && (static_cast<unsigned char>(text.back()) & 0xC0) == 0x80)
        text.pop_back();
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
