#include "pch.h"
#include "SceneView.h"
#include "Camera.h"
#include "EditorManager.h"

SceneView::SceneView()
    :Super("Scene")
{
}

SceneView::~SceneView()
{
}

void SceneView::OnGUI()
{
    Super::OnGUI();

    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    //ImVec2 contentMax = ImGui::GetWindowContentRegionMax();

    ImVec2 min = ImVec2(winPos.x + contentMin.x, winPos.y + contentMin.y);
    //ImVec2 max = ImVec2(winPos.x + contentMax.x, winPos.y + contentMax.y);

    // 창의 콘텐츠 영역 크기 (스크롤/패딩 제외)
    ImVec2 avail = ImGui::GetContentRegionAvail();
    UINT w = (UINT)max(10.0f, avail.x);
    UINT h = (UINT)max(10.0f, avail.y);

    GameDesc& gameDesc = GAME->GetGameDesc();
    gameDesc.sceneWidth = w;
    gameDesc.sceneHeight = h;
    gameDesc.scenePos = Vec2(min.x, min.y);
    gameDesc.sceneFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    ImGui::Image((ImTextureID)GRAPHICS->GetSceneViewSRV().Get(), avail);

    if(IsBegin)
        DrawSceneViewGizmoOverlay();

    // ImGuizmo
    TransformRef selectedTransformRef;
    _editorManager->TryGetHierarchyTransform(OUT selectedTransformRef);
    Transform* selectedTransform = selectedTransformRef.Resolve();
    GameObject* camObj = CUR_SCENE->GetMainCamera();
    if (camObj != nullptr && selectedTransform != nullptr)
    {
        Camera* camera = camObj->GetCamera();
        ImGuizmo::BeginFrame();
        ImGuizmo::AllowAxisFlip(false);
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(gameDesc.scenePos.x, gameDesc.scenePos.y, gameDesc.sceneWidth, gameDesc.sceneHeight);

        float snap[3] = { 0,0,0 };
        const float* snapPtr = nullptr;

        if (g_gizmo.useSnap)
        {
            if (g_gizmo.op == ImGuizmo::TRANSLATE) { snap[0] = snap[1] = snap[2] = g_gizmo.snapMove; snapPtr = snap; }
            else if (g_gizmo.op == ImGuizmo::ROTATE) { snap[0] = g_gizmo.snapRotate; snapPtr = snap; }
            else { snap[0] = snap[1] = snap[2] = g_gizmo.snapScale; snapPtr = snap; }
        }

        Matrix view = camera->GetViewMatrix();
        Matrix proj = camera->GetProjectionMatrix();
        Matrix world = selectedTransform->GetWorldMatrix();

        ImGuizmo::Manipulate(
            (float*)&view,
            (float*)&proj,
            g_gizmo.op,
            g_gizmo.mode,
            (float*)&world,
            nullptr,
            snapPtr
        );

        if (ImGuizmo::IsUsing())
        {
            selectedTransform->SetWorldMatrix(world);
        }
    }
}

void SceneView::DrawSceneViewGizmoOverlay()
{
    GameDesc& gameDesc = GAME->GetGameDesc();
    // 씬뷰 안쪽 여백
    const ImVec2 pad(8.0f, 8.0f);
    ImVec2 pos(gameDesc.scenePos.x + pad.x, gameDesc.scenePos.y + pad.y);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav;

    // 씬뷰 위에 "고정"으로 뜨게
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.85f);

    // 씬뷰 hover 중일 때만 클릭 허용하고 싶다면 아래처럼 토글 가능
    // if (!sceneHovered) flags |= ImGuiWindowFlags_NoInputs;

    ImGui::Begin("##SceneGizmoOverlay", nullptr, flags);

    auto DrawModeButton = [&](const char* label, ImGuizmo::OPERATION op)
        {
            bool selected = (g_gizmo.op == op);

            if (!selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            }

            if (ImGui::Button(label))
                g_gizmo.op = op;

            if (!selected)
                ImGui::PopStyleColor(3);
        };

    DrawModeButton("Move", ImGuizmo::TRANSLATE);
    ImGui::SameLine();
    DrawModeButton("Rot", ImGuizmo::ROTATE);
    ImGui::SameLine();
    DrawModeButton("Scale", ImGuizmo::SCALE);

    if (INPUT->GetButton(KEY_TYPE::RBUTTON) == false && INPUT->GetButton(KEY_TYPE::RBUTTON) == false)
    {
        if (INPUT->GetButtonDown(KEY_TYPE::W))
            g_gizmo.op = ImGuizmo::TRANSLATE;
        else if (INPUT->GetButtonDown(KEY_TYPE::E))
            g_gizmo.op = ImGuizmo::ROTATE;
        else if (INPUT->GetButtonDown(KEY_TYPE::R))
            g_gizmo.op = ImGuizmo::SCALE;
    }

    // ---- Mode
    ImGui::SameLine();
    bool isLocal = (g_gizmo.mode == ImGuizmo::LOCAL);
    if (ImGui::Checkbox("Local", &isLocal))
        g_gizmo.mode = isLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

    // ---- Snap
    ImGui::SameLine();
    ImGui::Checkbox("Snap", &g_gizmo.useSnap);

    if (g_gizmo.useSnap)
    {
        if (g_gizmo.op == ImGuizmo::TRANSLATE)
            ImGui::DragFloat("##snap_move", &g_gizmo.snapMove, 0.05f, 0.001f, 1000.0f, "Move %.3f");
        else if (g_gizmo.op == ImGuizmo::ROTATE)
            ImGui::DragFloat("##snap_rot", &g_gizmo.snapRotate, 1.0f, 0.1f, 180.0f, "Rot %.1f");
        else // scale
            ImGui::DragFloat("##snap_scale", &g_gizmo.snapScale, 0.01f, 0.001f, 10.0f, "Scale %.3f");
    }

    ImGui::End();
}
