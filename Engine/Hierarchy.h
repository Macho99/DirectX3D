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
    PendingReparent(TransformID droppedId, TransformID targetId, DropAction action);

    TransformID droppedId;
    TransformID targetId;
    DropAction action;

    void Do() override;
};

struct PendingDelete : PendingOperation
{
    PendingDelete(TransformID targetId);
    TransformID targetId;

    void Do() override;
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
    void ApplyPending();

private:
    TransformID _selectedId = -1;
    vector<unique_ptr<PendingOperation>> _pendingOps;
};

