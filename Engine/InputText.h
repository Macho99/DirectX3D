#pragma once
#include "Text.h"

class InputText : public Text
{
    using Super = Text;
    DECLARE_COMPONENT(InputText)

public:
    InputText();
    ~InputText();

    virtual void Update() override;
    virtual bool OnGUI() override;
    virtual void OnMouseDown() override;
    virtual void OnDisable() override;

    bool IsFocused() const { return _isFocused; }
    void SetFocused(bool focused);
    void SetInteractable(bool interactable);
    bool IsInteractable() const { return _interactable; }
    void SetMaxLength(uint32 maxLength);
    uint32 GetMaxLength() const { return _maxLength; }

    void AddOnValueChangedEvent(function<void(const string&)> callback);
    void AddOnSubmitEvent(function<void(const string&)> callback);

    virtual int GetVersion() const override { return Super::GetVersion() + 1; }

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_interactable));
        ar(CEREAL_NVP(_maxLength));
    }

private:
    void ProcessInput(const vector<uint32>& input);
    void UpdateCaret();
    void RestartCaretBlink();
    virtual bool ShouldRenderCaret() const override { return _isCaretVisible; }
    static void AppendUtf8(string& text, uint32 codepoint);
    static size_t GetUtf8Length(const string& text);
    static void PopUtf8Character(string& text);
    static void TruncateUtf8(string& text, size_t maxLength);

private:
    bool _isFocused = false;
    bool _interactable = true;
    uint32 _maxLength = 256;
    float _caretBlinkTimer = 0.0f;
    float _caretBlinkInterval = 0.5f;
    bool _isCaretVisible = false;
    function<void(const string&)> _onValueChanged;
    function<void(const string&)> _onSubmit;
};
