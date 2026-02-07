#include "pch.h"
#include "Hierarchy.h"
#include "EditorManager.h"

Hierarchy::Hierarchy()
    :Super("Hierarchy")
{
}

Hierarchy::~Hierarchy()
{
}

void Hierarchy::OnGUI()
{
    Super::OnGUI();

    _selectedId = _editorManager->GetSelectedTransform();
    ShowHierarchy();
    ApplyPending();
    _editorManager->SetSelectedTransform(_selectedId);
}

void Hierarchy::DrawInsertLine(const ImRect& r, bool top)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 col = ImGui::GetColorU32(ImGuiCol_PlotLinesHovered); // visible highlight

    float y = top ? r.Min.y : r.Max.y;
    float pad = 6.0f;
    dl->AddLine(ImVec2(r.Min.x + pad, y), ImVec2(r.Max.x - pad, y), col, 2.0f);
}

void Hierarchy::DrawChildHighlight(const ImRect& r)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 col = ImGui::GetColorU32(ImGuiCol_HeaderHovered);

    // Fill can cover text; if you prefer outline only, replace with AddRect(...)
    dl->AddRectFilled(r.Min, r.Max, col, 4.0f);
}

void Hierarchy::ShowHierarchy()
{
    // ===================== UI =====================
    shared_ptr<Scene> curScene = CUR_SCENE;

    // Right-click empty space menu
    if (ImGui::BeginPopupContextWindow("HierarchyEmptyContext",
        ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Empty Root"))
        {
            unique_ptr<GameObject> newObject = make_unique<GameObject>("New Object");
            curScene->Add(std::move(newObject));
        }
        ImGui::EndPopup();
    }

    ImGui::TextDisabled("Tip: Drag & drop. Top=before, Middle=child, Bottom=after. Drop on empty area => root.");

    Transform* selectedTransform = _selectedId.Resolve();
    if (selectedTransform != nullptr)
    {
        Transform* selectedParent;
        string selectedName = selectedTransform->GetGameObject()->GetName();
        if (selectedTransform->TryGetParent(OUT selectedParent))
        {
            Guid selectedParentId = selectedParent->GetGuid();
            ImGui::Text("Selected: %s (id=%llu_%llu, parent=%llu_%llu)", selectedName.c_str(), 
                _selectedId.GetInstanceId(), _selectedId.GetLocalId(), selectedParentId.GetInstanceId(), selectedParentId.GetLocalId());
        }
        else
        {
            ImGui::Text("Selected: %s (id=%llu_%llu)", selectedName.c_str(), _selectedId.GetInstanceId(), _selectedId.GetLocalId());
        }
    }
    else
    {
        ImGui::Text("Selected: (none)");
    }

    // Draw roots
    auto& roots = curScene->GetRootObjects();
    for (int i = 0; i < roots.size(); i++)
    {
        DrawNode(roots[i].Resolve());
    }

    // ----- Empty area drop target (VERY IMPORTANT) -----
    float availY = ImGui::GetContentRegionAvail().y;
    if (availY > 0.0f)
    {
        // 빈 영역을 차지하는 보이지 않는 아이템
        ImGui::InvisibleButton("##HierarchyEmptySpace", ImVec2(-1.0f, availY));

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("DND_ENTITY"))
            {
                TransformRef dropped = *(const TransformRef*)p->Data;

                _pendingOps.push_back(make_unique<PendingReparent>(dropped, TransformRef(), DropAction::MakeChild));
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            _selectedId = TransformRef();

        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            _selectedId = TransformRef();
    }
}

void Hierarchy::DrawNode(Transform* node)
{
    if (node == nullptr)
        return;

    TransformRef nodeId(node->GetGuid());
    GameObject* gameObject = node->GetGameObject();
    ImGui::PushID((int)nodeId.GetLocalId());

    string name = gameObject->GetName();

    // Active toggle
    bool active = gameObject->IsActive();
    ImGui::Checkbox("##active", &active);
    if (active != gameObject->IsActive()) 
        gameObject->SetActive(active);

    ImGui::SameLine();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanFullWidth |
        ImGuiTreeNodeFlags_FramePadding;

    if (_selectedId == nodeId) 
        flags |= ImGuiTreeNodeFlags_Selected;

    if (node->GetChildren().empty()) 
        flags |= ImGuiTreeNodeFlags_Leaf;

    bool open = ImGui::TreeNodeEx((void*)(intptr_t)nodeId.GetLocalId(), flags, "%s", string(name.begin(), name.end()).c_str());

    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        _selectedId = nodeId;

    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        _selectedId = nodeId;

    // Context menu on node
    if (ImGui::BeginPopupContextItem("NodeContext"))
    {
        if (ImGui::MenuItem("Toggle Active"))
            gameObject->SetActive(!gameObject->IsActive());

        if (ImGui::MenuItem("Delete"))
        {
            _pendingOps.push_back(make_unique<PendingDelete>(nodeId));
        }

        ImGui::EndPopup();
    }

    // Drag source
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("DND_ENTITY", &nodeId, sizeof(TransformRef));
        ImGui::Text("Move: %s", string(name.begin(), name.end()).c_str());
        ImGui::EndDragDropSource();
    }

    // Drop target with 3-zone preview + action
    if (ImGui::BeginDragDropTarget())
    {
        ImRect r(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        float h = r.GetHeight();
        float t0 = r.Min.y + h * 0.25f;
        float t1 = r.Min.y + h * 0.75f;

        float y = ImGui::GetIO().MousePos.y;

        DropAction dropAction;
        if (y < t0) dropAction = DropAction::InsertBefore;
        else if (y > t1) dropAction = DropAction::InsertAfter;
        else dropAction = DropAction::MakeChild;

        // Preview drawing
        if (dropAction == DropAction::InsertBefore)
            DrawInsertLine(r, true);
        else if (dropAction == DropAction::InsertAfter)
            DrawInsertLine(r, false);
        else
            DrawChildHighlight(r);

        // Commit on drop
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("DND_ENTITY"))
        {
            TransformRef dropped = *(const TransformRef*)p->Data;
            _pendingOps.push_back(make_unique<PendingReparent>(dropped, nodeId, dropAction));
        }

        ImGui::EndDragDropTarget();
    }

    if (open)
    {
        vector<TransformRef>& children = node->GetChildren();
        for (int i = 0; i < children.size(); i++)
            DrawNode(children[i].Resolve());
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void Hierarchy::ApplyPending()
{
    for (auto& op : _pendingOps)
    {
        op->Do();
    }
    _pendingOps.clear();
}

PendingReparent::PendingReparent(TransformRef droppedId, TransformRef targetId, DropAction action)
    :droppedId(droppedId), targetId(targetId), action(action)
{
}

void PendingReparent::Do()
{
    Transform* droppedTransform = droppedId.Resolve();
    Transform* targetTransform = targetId.Resolve();
    if (droppedTransform == nullptr) 
        return;

    if (targetTransform == nullptr || action == DropAction::MakeChild)
    {
        droppedTransform->SetParent(targetId);
    }
    else
    {
        Transform* parent = targetTransform->GetParent();
        TransformRef parentRef;
        if (parent == nullptr)
            parentRef = TransformRef();
        else
            parentRef = TransformRef(parent->GetGuid());
        droppedTransform->SetParent(parentRef);

        // siblings는 parent 기준/루트 기준으로 다시 계산
        vector<TransformRef>& siblings = parent ? parent->GetChildren()
            : CUR_SCENE->GetRootObjects();

        int idx = int(std::find(siblings.begin(), siblings.end(), targetId) - siblings.begin());
        if (action == DropAction::InsertAfter) idx++;
        droppedTransform->SetSiblingIndex(idx);
    }
}

PendingDelete::PendingDelete(TransformRef targetId)
    :targetId(targetId)
{
}

void PendingDelete::Do()
{
    Transform* targetTransform = targetId.Resolve();
    if (targetTransform == nullptr)
        return;

    CUR_SCENE->Remove(targetTransform->GetGameObject()->GetGuid());
}
