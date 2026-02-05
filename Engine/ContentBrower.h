#pragma once
#include "EditorWindow.h"
class ContentBrower : public EditorWindow
{
    using Super = EditorWindow;

public:
    ContentBrower();
    ~ContentBrower();

protected:
    void OnGUI() override;
};

