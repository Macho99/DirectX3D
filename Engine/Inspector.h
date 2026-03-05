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

    void DrawCard(string title, const void* const idPtr, function<void()> onGui);

private:
    void DrawComponentCard(Component& component);
};

