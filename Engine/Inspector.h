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
    void DrawGameObject(TransformRef& transformRef);
    void DrawAsset(AssetRef& assetRef, int subAssetIdx);

    bool DrawCard(string title, const void* const idPtr, function<bool()> onGui, function<void()> onMenu);

private:
    void DrawComponentCard(Component& component);

private:
    //Vec2 _testValue = Vec2(0.f);
};

