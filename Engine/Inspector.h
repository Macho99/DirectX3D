#pragma once
#include "EditorWindow.h"

template<class T>
class InspectorDrawer;

class IInspectorDrawer;

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
    template<class T>
    void Register(std::unique_ptr<InspectorDrawer<T>> drawer)
    {
        _map[typeid(T)] = std::move(drawer);
    }

    void Draw(Component& component);
    void DrawComponentCard(Component& component);
private:
    unordered_map<std::type_index, unique_ptr<IInspectorDrawer>> _map;
};

