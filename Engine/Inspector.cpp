#include "pch.h"
#include "Inspector.h"
#include "InspectorDrawer.h"
#include "TransformDrawer.h"
#include "EditorManager.h"
#include "MonoBehaviour.h"

Inspector::Inspector()
    :Super("Inspector")
{
}

Inspector::~Inspector()
{
}

void Inspector::Init(EditorManager* editorManager)
{
    Super::Init(editorManager);

    Register<Transform>(make_unique<TransformDrawer>());
}

void Inspector::OnGUI()
{
    Super::OnGUI();

    TransformRef selectedTransform = _editorManager->GetSelectedTransform();
    Transform* transform = selectedTransform.Resolve();
    if (transform == nullptr)
    {
        ImGui::Text("No selection");
        return;
    }

    GameObject* gameObject = transform->GetGameObject();
    ImGui::Text("GameObject: %s", gameObject->GetName().c_str());
    ImGui::Separator();
    auto& components = gameObject->GetAllFixedComponents();
    for (auto& compRef : components)
    {
        Component* component = compRef.Resolve();
        if (component == nullptr)
            continue; 

        DrawComponentCard(*component);
    }
    for (auto& scriptRef : gameObject->GetScripts())
    {
        Component* component = static_cast<Component*>(scriptRef.Resolve());
        if (component == nullptr)
            continue;

        DrawComponentCard(*component);
    }
}

void Inspector::Draw(Component& component)
{
    auto it = _map.find(typeid(component));
    if (it == _map.end())
    {
        ImGui::Text("No inspector for component type: %s", typeid(component).name());
        return;
    }

    it->second->Draw(component);
}

void Inspector::DrawComponentCard(Component& component)
{
    ImGui::PushID(&component);
    ImGui::Spacing();

    // 카드 테두리용 그룹
    ImVec2 start = ImGui::GetCursorScreenPos();
    ImGui::BeginGroup();

    // 헤더 프레임처럼 보이게
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_OpenOnArrow; // 화살표 클릭으로 열기(유니티 느낌)

    const char* title = typeid(component).name();
    bool open = ImGui::TreeNodeEx("##Header", flags, "%s", title);

    // 헤더 오른쪽에 버튼 배치
    // TreeNodeEx가 그린 "헤더 영역"의 오른쪽 끝 좌표를 이용합니다.
    ImVec2 rmin = ImGui::GetItemRectMin();
    ImVec2 rmax = ImGui::GetItemRectMax();

    float btnW = 22.0f;
    float padR = 6.0f;
    float x = rmax.x - padR - btnW;
    float y = rmin.y + (rmax.y - rmin.y - ImGui::GetFrameHeight()) * 0.5f;

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));

    ImGui::SetCursorScreenPos(ImVec2(x, y));
    if (ImGui::SmallButton("..."))
        ImGui::OpenPopup("##CompMenu");

    ImGui::PopStyleColor(3);

    if (ImGui::BeginPopup("##CompMenu"))
    {
        ImGui::TextDisabled("Component Menu");
        ImGui::EndPopup();
    }

    if (open)
    {
        ImGui::Indent(8.0f);
        ImGui::Separator();
        Draw(component);
        ImGui::Unindent(8.0f);
        ImGui::TreePop();
    }

    ImGui::EndGroup();
    ImVec2 end = ImGui::GetItemRectMax();
    start.x -= 4.0f;

    // 수동 테두리
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRect(start, end, ImGui::GetColorU32(ImGuiCol_Border), 4.0f);

    ImGui::PopID();
}
