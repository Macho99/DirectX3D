#include "pch.h"
#include "Inspector.h"
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
}

void Inspector::OnGUI()
{
    Super::OnGUI();
    DrawGameObject();
    DrawAsset();
}

void Inspector::DrawGameObject()
{
    TransformRef selectedTransform;
    _editorManager->TryGetSelectedTransform(OUT selectedTransform);
    Transform* transform = selectedTransform.Resolve();
    if (transform == nullptr)
    {
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

void Inspector::DrawAsset()
{
    AssetRef selectedAsset;
    int selectedSubAssetIndex;
    if (_editorManager->TryGetSelectedAsset(OUT selectedAsset, OUT selectedSubAssetIndex) == false)
        return;

    MetaFile* meta = nullptr;
    if (RESOURCES->TryGetMetaByAssetId(selectedAsset.GetAssetId(), meta) == false)
    {
        DBG->LogErrorW(L"[Inspector] DrawAsset: Failed to get meta for selected asset: " + selectedAsset.GetAssetId().ToWString());
        return;
    }

    switch (meta->GetResourceType())
    {
    case ResourceType::Folder:
    case ResourceType::None:
        return;
    }

    ResourceBase* resource;
    if (selectedSubAssetIndex == -1)
    {
        resource = selectedAsset.Resolve();
    }
    else
    {
        AssetRef selectedSubAsset(static_cast<SubAssetMetaFile*>(meta)->GetSubAssetIdByIndex(selectedSubAssetIndex));
        resource = selectedSubAsset.Resolve();
    }

    if (resource == nullptr)
    {
        DBG->LogErrorW(L"[Inspector] DrawAsset: Failed to resolve selected asset: " + selectedAsset.GetAssetId().ToWString());
        return;
    }

    DrawCard(typeid(*resource).name(), &resource, [&]() { resource->OnGUI(); });
    DrawCard(typeid(*meta).name(), &meta, [&]() { meta->OnGUI(); });
}
void Inspector::DrawCard(string title, const void* const idPtr, function<void()> onGui)
{
    ImGui::PushID(idPtr);
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

    bool open = ImGui::TreeNodeEx("##Header", flags, "%s", title.c_str());

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
        onGui();
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

void Inspector::DrawComponentCard(Component& component)
{
    DrawCard(typeid(component).name(), &component, [&]
        {
            component.OnGUI();
        });
}
