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

struct PendingOperation
{
    virtual ~PendingOperation() = default;
    virtual void Do() = 0;
};

struct PendingReparent : PendingOperation
{
    PendingReparent(TransformRef droppedId, TransformRef targetId, DropAction action);

    TransformRef droppedId;
    TransformRef targetId;
    DropAction action;

    void Do() override;
};

struct PendingDelete : PendingOperation
{
    PendingDelete(TransformRef targetId);
    TransformRef targetId;

    void Do() override;
};

class Hierarchy : public EditorWindow
{
    using Super = EditorWindow;
public:
    Hierarchy();
    ~Hierarchy();

protected:
    void OnGUI() override;

private:
    void DrawInsertLine(const ImRect& r, bool top);
    void DrawChildHighlight(const ImRect& r);
    void ShowHierarchy();
    void DrawNode(Transform* node);
    void ApplyPending();

private:
    TransformRef _selectedId;
    vector<unique_ptr<PendingOperation>> _pendingOps;
};

