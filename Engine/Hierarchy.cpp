#include "pch.h"
#include "Hierarchy.h"

void Hierarchy::Init()
{
}

static void ShowHierarchySample();

void Hierarchy::OnGUI()
{
    Super::OnGUI();

    ShowHierarchySample();
}

static void DrawInsertLine(const ImRect& r, bool top)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 col = ImGui::GetColorU32(ImGuiCol_PlotLinesHovered); // visible highlight

    float y = top ? r.Min.y : r.Max.y;
    float pad = 6.0f;
    dl->AddLine(ImVec2(r.Min.x + pad, y), ImVec2(r.Max.x - pad, y), col, 2.0f);
}

static void DrawChildHighlight(const ImRect& r)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 col = ImGui::GetColorU32(ImGuiCol_HeaderHovered);

    // Fill can cover text; if you prefer outline only, replace with AddRect(...)
    dl->AddRectFilled(r.Min, r.Max, col, 4.0f);
}

static void ShowHierarchySample()
{
    using Entity = uint32_t;

    struct Node
    {
        Entity id = 0;
        Entity parent = 0;                 // 0 = root
        std::vector<Entity> children;      // ordered
        std::string name;
        bool active = true;
    };

    enum class DropAction
    {
        InsertBefore,
        InsertAfter,
        MakeChild
    };

    // ---------- sample scene (static) ----------
    static std::unordered_map<Entity, Node> g_nodes;
    static std::vector<Entity> g_roots;
    static Entity g_selected = 0;
    static Entity g_nextId = 100;

    auto GetNode = [&](Entity id) -> Node*
        {
            auto it = g_nodes.find(id);
            return (it == g_nodes.end()) ? nullptr : &it->second;
        };

    auto RemoveFromVec = [&](std::vector<Entity>& v, Entity x)
        {
            auto it = std::find(v.begin(), v.end(), x);
            if (it != v.end()) v.erase(it);
        };

    auto InitIfNeeded = [&]()
        {
            if (!g_nodes.empty()) return;

            auto Add = [&](Entity id, const char* name)
                {
                    Node n;
                    n.id = id;
                    n.name = name;
                    g_nodes[id] = n;
                };

            Add(1, "Scene");
            Add(2, "Player");
            Add(3, "Camera");
            Add(4, "Weapon");
            Add(5, "Enemies");
            Add(6, "Goblin");
            Add(7, "Orc");
            Add(8, "Light");
            Add(9, "UI");

            g_roots = { 1, 9 };

            // Scene children
            g_nodes[1].children = { 2, 3, 5, 8 };
            g_nodes[2].parent = 1;
            g_nodes[3].parent = 1;
            g_nodes[5].parent = 1;
            g_nodes[8].parent = 1;

            // Player children
            g_nodes[2].children = { 4 };
            g_nodes[4].parent = 2;

            // Enemies children
            g_nodes[5].children = { 6, 7 };
            g_nodes[6].parent = 5;
            g_nodes[7].parent = 5;

            // UI root
            g_nodes[9].parent = 0;

            g_selected = 2;
        };

    InitIfNeeded();

    // --------- cycle check: is `node` inside subtree of `ancestor`? ----------
    auto IsDescendant = [&](Entity node, Entity ancestor) -> bool
        {
            if (node == 0 || ancestor == 0) return false;
            Node* a = GetNode(ancestor);
            if (!a) return false;

            std::vector<Entity> stack;
            stack.push_back(ancestor);
            while (!stack.empty())
            {
                Entity cur = stack.back();
                stack.pop_back();
                if (cur == node) return true;

                Node* cn = GetNode(cur);
                if (!cn) continue;
                for (Entity c : cn->children) stack.push_back(c);
            }
            return false;
        };

    // ---------- Reparent + reorder with action ----------
    auto ReparentWithAction = [&](Entity child, Entity target, DropAction action)
        {
            if (child == 0) return;
            if (child == target) return;

            Node* c = GetNode(child);
            if (!c) return;

            Entity newParent = 0;
            std::vector<Entity>* newContainer = nullptr; // roots or parent's children
            int insertIndex = -1;

            if (action == DropAction::MakeChild)
            {
                newParent = target;

                // cycle prevention: target cannot be inside child's subtree
                if (newParent != 0 && IsDescendant(newParent, child))
                    return;

                Node* np = GetNode(newParent);
                if (!np) return;

                newContainer = &np->children;
                insertIndex = (int)newContainer->size(); // append
            }
            else
            {
                Node* t = GetNode(target);
                if (!t) return;

                newParent = t->parent;

                if (newParent == 0)
                {
                    newContainer = &g_roots;
                }
                else
                {
                    Node* np = GetNode(newParent);
                    if (!np) return;
                    newContainer = &np->children;
                }

                // find target index in destination container
                auto itT = std::find(newContainer->begin(), newContainer->end(), target);
                if (itT == newContainer->end()) return;

                int tIndex = (int)std::distance(newContainer->begin(), itT);
                insertIndex = (action == DropAction::InsertBefore) ? tIndex : (tIndex + 1);

                // cycle prevention: newParent cannot be inside child's subtree
                if (newParent != 0 && IsDescendant(newParent, child))
                    return;
            }

            // old container
            Entity oldParent = c->parent;
            std::vector<Entity>* oldContainer = nullptr;
            if (oldParent == 0) oldContainer = &g_roots;
            else
            {
                Node* op = GetNode(oldParent);
                oldContainer = op ? &op->children : nullptr;
            }
            if (!oldContainer) return;

            // old index (for same-container adjustment)
            int oldIndex = -1;
            {
                auto itOld = std::find(oldContainer->begin(), oldContainer->end(), child);
                if (itOld != oldContainer->end())
                    oldIndex = (int)std::distance(oldContainer->begin(), itOld);
            }

            const bool sameContainer = (oldContainer == newContainer);

            // remove from old container
            {
                auto itOld = std::find(oldContainer->begin(), oldContainer->end(), child);
                if (itOld != oldContainer->end())
                    oldContainer->erase(itOld);
            }

            // adjust insert index if moving forward within same container
            if (sameContainer && oldIndex != -1 && oldIndex < insertIndex)
                insertIndex--;

            // clamp
            if (insertIndex < 0) insertIndex = 0;
            if (insertIndex > (int)newContainer->size()) insertIndex = (int)newContainer->size();

            // update parent + insert
            c->parent = newParent;
            newContainer->insert(newContainer->begin() + insertIndex, child);
        };

    // ---------- Delete subtree (demo convenience) ----------
    auto DeleteSubtree = [&](Entity id)
        {
            Node* n = GetNode(id);
            if (!n) return;

            // collect
            std::vector<Entity> toDelete;
            std::vector<Entity> stack = { id };
            while (!stack.empty())
            {
                Entity cur = stack.back(); stack.pop_back();
                Node* cn = GetNode(cur);
                if (!cn) continue;
                for (Entity c : cn->children) stack.push_back(c);
                toDelete.push_back(cur);
            }

            // detach root/parent link for the top node
            Entity parent = n->parent;
            if (parent == 0) RemoveFromVec(g_roots, id);
            else
            {
                Node* p = GetNode(parent);
                if (p) RemoveFromVec(p->children, id);
            }

            // erase
            for (Entity e : toDelete) g_nodes.erase(e);
            if (g_selected == id) g_selected = 0;
        };

    // ===================== UI =====================
    ImGui::Begin("Hierarchy");

    // Right-click empty space menu
    if (ImGui::BeginPopupContextWindow("HierarchyEmptyContext",
        ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Empty Root"))
        {
            Entity id = g_nextId++;
            Node n;
            n.id = id;
            n.parent = 0;
            n.name = std::string("Empty ") + std::to_string(id);
            g_nodes[id] = n;
            g_roots.push_back(id);
        }
        ImGui::EndPopup();
    }

    ImGui::TextDisabled("Tip: Drag & drop. Top=before, Middle=child, Bottom=after. Drop on empty area => root.");

    if (g_selected != 0)
    {
        Node* s = GetNode(g_selected);
        if (s)
            ImGui::Text("Selected: %s (id=%u, parent=%u)", s->name.c_str(), s->id, s->parent);
        else
            ImGui::Text("Selected: (missing)");
    }
    else
    {
        ImGui::Text("Selected: (none)");
    }

    // Recursive draw
    std::function<void(Entity)> DrawNode;
    DrawNode = [&](Entity id)
        {
            Node* n = GetNode(id);
            if (!n) return;

            ImGui::PushID((int)n->id);

            // Active toggle
            bool active = n->active;
            ImGui::Checkbox("##active", &active);
            if (active != n->active) n->active = active;
            ImGui::SameLine();

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanFullWidth |
                ImGuiTreeNodeFlags_FramePadding;

            if (g_selected == n->id) flags |= ImGuiTreeNodeFlags_Selected;
            if (n->children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;

            bool open = ImGui::TreeNodeEx((void*)(intptr_t)n->id, flags, "%s", n->name.c_str());

            // Click select
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                g_selected = n->id;

            // Context menu on node
            if (ImGui::BeginPopupContextItem("NodeContext"))
            {
                if (ImGui::MenuItem("Rename"))
                    n->name += " (Renamed)";

                if (ImGui::MenuItem("Toggle Active"))
                    n->active = !n->active;

                if (ImGui::MenuItem("Delete Subtree"))
                {
                    Entity delId = n->id;
                    ImGui::EndPopup();

                    if (open) ImGui::TreePop();
                    ImGui::PopID();

                    DeleteSubtree(delId);
                    return;
                }

                ImGui::EndPopup();
            }

            // Drag source
            if (ImGui::BeginDragDropSource())
            {
                Entity payload = n->id;
                ImGui::SetDragDropPayload("DND_ENTITY", &payload, sizeof(Entity));
                ImGui::Text("Move: %s", n->name.c_str());
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

                DropAction previewAction;
                if (y < t0) previewAction = DropAction::InsertBefore;
                else if (y > t1) previewAction = DropAction::InsertAfter;
                else previewAction = DropAction::MakeChild;

                // Preview drawing
                if (previewAction == DropAction::InsertBefore)
                    DrawInsertLine(r, true);
                else if (previewAction == DropAction::InsertAfter)
                    DrawInsertLine(r, false);
                else
                    DrawChildHighlight(r);

                // Commit on drop
                if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("DND_ENTITY"))
                {
                    Entity dropped = *(const Entity*)p->Data;

                    DropAction action;
                    if (y < t0) action = DropAction::InsertBefore;
                    else if (y > t1) action = DropAction::InsertAfter;
                    else action = DropAction::MakeChild;

                    ReparentWithAction(dropped, n->id, action);
                }

                ImGui::EndDragDropTarget();
            }

            if (open)
            {
                for (Entity c : n->children)
                    DrawNode(c);
                ImGui::TreePop();
            }

            ImGui::PopID();
        };

    // Draw roots
    for (Entity r : g_roots)
        DrawNode(r);

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
                Entity dropped = *(const Entity*)p->Data;

                // make root (append)
                Node* c = GetNode(dropped);
                if (c)
                {
                    // 기존 부모에서 제거
                    if (c->parent == 0)
                        RemoveFromVec(g_roots, dropped);
                    else
                    {
                        Node* op = GetNode(c->parent);
                        if (op) RemoveFromVec(op->children, dropped);
                    }

                    c->parent = 0;
                    g_roots.push_back(dropped);
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    ImGui::End();
}
