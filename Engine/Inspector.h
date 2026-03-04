#pragma once
#include "EditorWindow.h"

class ResourceDrawerBase;
class ComponentDrawerBase;

class Inspector : public EditorWindow
{
    using Super = EditorWindow;
public:
    Inspector();
    ~Inspector();

protected:
    void Init(class EditorManager* editorManager) override;
    void OnGUI() override;

private:
    void DrawComponentCard(Component& component);
};

