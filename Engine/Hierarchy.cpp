#include "pch.h"
#include "Hierarchy.h"

Hierarchy::Hierarchy()
{
}

Hierarchy::~Hierarchy()
{
}

void Hierarchy::Init()
{
}

void Hierarchy::OnGUI()
{
    Super::OnGUI();

    ShowHierarchy();
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
    ImGui::Begin("Hierarchy");
    shared_ptr<Scene> curScene = CUR_SCENE;

    // Right-click empty space menu
    if (ImGui::BeginPopupContextWindow("HierarchyEmptyContext",
        ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Empty Root"))
        {
            shared_ptr<GameObject> newObject = make_shared<GameObject>(L"New Object");
            curScene->Add(newObject);
        }
        ImGui::EndPopup();
    }

    ImGui::TextDisabled("Tip: Drag & drop. Top=before, Middle=child, Bottom=after. Drop on empty area => root.");

    if (_selectedId != -1)
    {
        shared_ptr<Transform> selectedTransform;
        if (curScene->TryGetTransform(_selectedId, selectedTransform))
        {
            shared_ptr<Transform> selectedParent;
            wstring selectedName = selectedTransform->GetGameObject()->GetName();
            if (selectedTransform->TryGetParent(OUT selectedParent))
            {
                TransformID parentId = selectedParent->GetID();
                ImGui::Text("Selected: %s (id=%d, parent=%d)", string(selectedName.begin(), selectedName.end()).c_str(), selectedTransform->GetID(), parentId);
            }
            else
            {
                ImGui::Text("Selected: %s (id=%d)", string(selectedName.begin(), selectedName.end()).c_str(), selectedTransform->GetID());
            }
        }
        else
        {
            ImGui::Text("Selected: (missing)");
            _selectedId = -1;
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
        DrawNode(roots[i]);
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
                TransformID dropped = *(const TransformID*)p->Data;

                shared_ptr<Transform> findTransform;
                if (curScene->TryGetTransform(dropped, OUT findTransform))
                {
                    findTransform->SetParent(nullptr);
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    ImGui::End();
}

void Hierarchy::DrawNode(shared_ptr<Transform>& node)
{
    if (node == nullptr)
        return;

    TransformID nodeId = node->GetID();
    shared_ptr<GameObject> gameObject = node->GetGameObject();
    ImGui::PushID((int)nodeId);

    wstring name = gameObject->GetName();

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

    bool open = ImGui::TreeNodeEx((void*)(intptr_t)node->GetID(), flags, "%s", string(name.begin(), name.end()).c_str());

    // Click select
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        _selectedId = nodeId;

    // Context menu on node
    if (ImGui::BeginPopupContextItem("NodeContext"))
    {
        if (ImGui::MenuItem("Rename"))
            gameObject->SetName(name + L" (Renamed)");

        if (ImGui::MenuItem("Toggle Active"))
            gameObject->SetActive(!gameObject->IsActive());

        if (ImGui::MenuItem("Delete Subtree"))
        {
            wcout << "Delete TODO" << endl;
        }

        ImGui::EndPopup();
    }

    // Drag source
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("DND_ENTITY", &nodeId, sizeof(TransformID));
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
            TransformID dropped = *(const TransformID*)p->Data;
            shared_ptr<Transform> droppedTransform;

            CUR_SCENE->TryGetTransform(dropped, OUT droppedTransform);

            if (dropAction == DropAction::MakeChild)
                droppedTransform->SetParent(node);
            else
            {
                vector<shared_ptr<Transform>>* siblings;
                shared_ptr<Transform> parent = nullptr;
                if (node->TryGetParent(parent))
                {
                    siblings = &parent->GetChildren();
                }
                else
                {
                    siblings = &CUR_SCENE->GetRootObjects();
                }
                
                int siblingIdx = std::find(siblings->begin(), siblings->end(), node) - siblings->begin();

                droppedTransform->SetParent(node->GetParent());
                if (dropAction == DropAction::InsertAfter)
                {
                    siblingIdx++;
                }
                droppedTransform->SetSiblingIndex(siblingIdx);
            }
        }

        ImGui::EndDragDropTarget();
    }

    if (open)
    {
        vector<shared_ptr<Transform>>& children = node->GetChildren();
        for (int i = 0; i < children.size(); i++)
            DrawNode(children[i]);
        ImGui::TreePop();
    }

    ImGui::PopID();
}
