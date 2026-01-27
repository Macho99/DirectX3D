#pragma once
#include "EditorWindow.h"

struct ImRect 
{ 
    ImVec2 Min, Max; 

    float GetHeight() const { return Max.y - Min.y; }
};

enum class DropAction
{
    InsertBefore,
    InsertAfter,
    MakeChild
};

class Hierarchy : public EditorWindow
{
    using Super = EditorWindow;
public:
    Hierarchy();
    ~Hierarchy();
    void Init() override;

protected:
    void OnGUI() override;

private:
    void DrawInsertLine(const ImRect& r, bool top);
    void DrawChildHighlight(const ImRect& r);
    void ShowHierarchy();
    void DrawNode(shared_ptr<Transform>& node);

private:
    TransformID _selectedId;
};

