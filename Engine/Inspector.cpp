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

    //ImGui::DragFloat2("Test Float", &_testValue.x, 0.1f);

    TransformRef selectedTransform;
    AssetRef selectedAsset;
    int selectedSubAssetIndex;
    _editorManager->GetInspectorRef(OUT selectedTransform, OUT selectedAsset, OUT selectedSubAssetIndex);

    DrawGameObject(selectedTransform);
    DrawAsset(selectedAsset, selectedSubAssetIndex);
}

void Inspector::DrawGameObject(TransformRef& transformRef)
{
    Transform* transform = transformRef.Resolve();
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

void Inspector::DrawAsset(AssetRef& assetRef, int subAssetIdx)
{
    if (assetRef.IsValid() == false)
        return;

    MetaFile* meta = nullptr;
    if (RESOURCES->TryGetMetaByAssetId(assetRef.GetAssetId(), meta) == false)
    {
        DBG->LogErrorW(L"[Inspector] DrawAsset: Failed to get meta for selected asset: " + assetRef.GetAssetId().ToWString());
        return;
    }

    switch (meta->GetResourceType())
    {
    case ResourceType::Folder:
    case ResourceType::None:
        return;
    }

    bool readOnly = false;
    ResourceBase* resource;
    if (subAssetIdx == -1)
    {
        resource = assetRef.Resolve();
    }
    else
    {
        readOnly = true;
        AssetRef selectedSubAsset(static_cast<SubAssetMetaFile*>(meta)->GetSubAssetIdByIndex(subAssetIdx));
        resource = selectedSubAsset.Resolve();
        DrawCard(typeid(*resource).name(), &resource, [&]() { return resource->OnGUI(true); });
        return;
    }

    if (resource == nullptr)
    {
        DBG->LogErrorW(L"[Inspector] DrawAsset: Failed to resolve selected asset: " + assetRef.GetAssetId().ToWString());
        return;
    }

    bool resourceChanged = DrawCard(typeid(*resource).name(), &resource, [&]() { return resource->OnGUI(false); });
    if (resourceChanged)
    {
        RESOURCES->SaveAsset(assetRef.GetAssetId());
    }
    DrawCard(typeid(*meta).name(), &meta, [&]() { return meta->OnGUI(); });
}

bool Inspector::DrawCard(string title, const void* const idPtr, function<bool()> onGui)
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
        ImGuiTreeNodeFlags_AllowOverlap |
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
    if (ImGui::Button("...", ImVec2(btnW, 23.f)))
    {
        ImGui::OpenPopup("##CompMenu");
    }

    ImGui::PopStyleColor(3);

    if (ImGui::BeginPopup("##CompMenu"))
    {
        ImGui::TextDisabled("Component Menu");
        ImGui::EndPopup();
    }

    bool changed = false;
    if (open)
    {
        ImGui::Indent(8.f);
        ImGui::Dummy(ImVec2(0, 4));
        changed = onGui();
        ImGui::Dummy(ImVec2(0, 8));
        ImGui::Unindent(8.f);
        ImGui::TreePop();
    }

    ImGui::EndGroup();
    ImVec2 end = ImGui::GetItemRectMax();
    end.x += 7.2f;
    start.x -= 4.0f;

    // 수동 테두리
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRect(start, end, ImGui::GetColorU32(ImGuiCol_Border), 4.0f);

    ImGui::PopID();
    return changed;
}

void Inspector::DrawComponentCard(Component& component)
{
    DrawCard(typeid(component).name(), &component, [&]
        {
            return component.OnGUI();
        });
}
