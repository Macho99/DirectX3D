#include "pch.h"
#include "SceneView.h"
#include "Camera.h"
#include "EditorManager.h"
#include "RectTransform.h"
#include "Viewport.h"

namespace
{
    enum class RectTransformHandleType
    {
        Move,
        Left,
        Right,
        Bottom,
        Top,
    };
}

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

    // ImGuizmo
    TransformRef selectedTransformRef;
    _editorManager->TryGetHierarchyTransform(OUT selectedTransformRef);
    Transform* selectedTransform = selectedTransformRef.Resolve();
    if (selectedTransform != nullptr)
    {
        GameObject* camObj = CUR_SCENE->GetMainCamera();
        GameObject* uiCamObj = CUR_SCENE->GetUICamera();

        RectTransform* rectTransform = dynamic_cast<RectTransform*>(selectedTransform);

        if (camObj != nullptr && rectTransform == nullptr)
        {
            Camera* camera = camObj->GetCamera();
            DrawTransformGizmo(selectedTransform, camera);
        }
        else if (uiCamObj != nullptr && rectTransform != nullptr)
        {
            Camera* camera = uiCamObj->GetCamera();
            DrawRectTransformGizmo(rectTransform, camera);
        }
    }

    if(IsBegin)
        DrawSceneViewGizmoOverlay();
}

void SceneView::DrawTransformGizmo(Transform* selectedTransform, Camera* camera)
{
    GameDesc& gameDesc = GAME->GetGameDesc();

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
        gameDesc.sceneFocused = false;
        selectedTransform->SetWorldMatrix(world);
    }
}

void SceneView::DrawRectTransformGizmo(RectTransform* selectedTransform, Camera* camera)
{
    RectTransform* rectTransform = dynamic_cast<RectTransform*>(selectedTransform);
    if (rectTransform == nullptr || camera == nullptr)
        return;

    GameDesc& gameDesc = GAME->GetGameDesc();
    Viewport viewport(
        static_cast<float>(gameDesc.sceneWidth),
        static_cast<float>(gameDesc.sceneHeight),
        gameDesc.scenePos.x,
        gameDesc.scenePos.y);

    const Vec2& pivot = rectTransform->GetPivot();
    const Vec3 localCorners[4] =
    {
        Vec3(-pivot.x, -pivot.y, 0.f),
        Vec3(1.f - pivot.x, -pivot.y, 0.f),
        Vec3(1.f - pivot.x, 1.f - pivot.y, 0.f),
        Vec3(-pivot.x, 1.f - pivot.y, 0.f),
    };

    Matrix world = rectTransform->GetWorldMatrix();
    Matrix view = camera->GetViewMatrix();
    Matrix projection = camera->GetProjectionMatrix();

    ImVec2 screenCorners[4] = {};
    for (int i = 0; i < 4; ++i)
    {
        Vec3 projected = viewport.Project(localCorners[i], world, view, projection);
        screenCorners[i] = ImVec2(projected.x, projected.y);
    }

    Vec3 pivotPosition = viewport.Project(Vec3::Zero, world, view, projection);
    ImVec2 screenPivot(pivotPosition.x, pivotPosition.y);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImU32 rectColor = IM_COL32(80, 200, 255, 255);
    const ImU32 pivotColor = IM_COL32(255, 230, 120, 255);
    const ImU32 handleColor = IM_COL32(80, 200, 255, 255);
    const ImU32 handleHoverColor = IM_COL32(255, 230, 120, 255);
    const ImU32 handleActiveColor = IM_COL32(255, 150, 80, 255);
    drawList->AddPolyline(screenCorners, IM_ARRAYSIZE(screenCorners), rectColor, ImDrawFlags_Closed, 2.0f);
    drawList->AddLine(ImVec2(screenPivot.x - 5.f, screenPivot.y), ImVec2(screenPivot.x + 5.f, screenPivot.y), pivotColor, 1.5f);
    drawList->AddLine(ImVec2(screenPivot.x, screenPivot.y - 5.f), ImVec2(screenPivot.x, screenPivot.y + 5.f), pivotColor, 1.5f);

    const ImVec2 handlePositions[] =
    {
        screenPivot,
        ImVec2((screenCorners[0].x + screenCorners[3].x) * 0.5f, (screenCorners[0].y + screenCorners[3].y) * 0.5f),
        ImVec2((screenCorners[1].x + screenCorners[2].x) * 0.5f, (screenCorners[1].y + screenCorners[2].y) * 0.5f),
        ImVec2((screenCorners[0].x + screenCorners[1].x) * 0.5f, (screenCorners[0].y + screenCorners[1].y) * 0.5f),
        ImVec2((screenCorners[2].x + screenCorners[3].x) * 0.5f, (screenCorners[2].y + screenCorners[3].y) * 0.5f),
    };
    const RectTransformHandleType handleTypes[] =
    {
        RectTransformHandleType::Move,
        RectTransformHandleType::Left,
        RectTransformHandleType::Right,
        RectTransformHandleType::Bottom,
        RectTransformHandleType::Top,
    };

    const float handleSize = 10.f;
    const float halfHandleSize = handleSize * 0.5f;
    ImGui::PushID(rectTransform);
    for (int i = 0; i < IM_ARRAYSIZE(handlePositions); ++i)
    {
        ImGui::PushID(i);
        ImVec2 handlePos = handlePositions[i];
        ImGui::SetCursorScreenPos(ImVec2(handlePos.x - halfHandleSize, handlePos.y - halfHandleSize));
        ImGui::InvisibleButton("##RectTransformHandle", ImVec2(handleSize, handleSize));

        ImU32 color = handleColor;
        if (ImGui::IsItemActive())
            color = handleActiveColor;
        else if (ImGui::IsItemHovered())
            color = handleHoverColor;

        drawList->AddRectFilled(
            ImVec2(handlePos.x - halfHandleSize, handlePos.y - halfHandleSize),
            ImVec2(handlePos.x + halfHandleSize, handlePos.y + halfHandleSize),
            color,
            1.5f);
        drawList->AddRect(
            ImVec2(handlePos.x - halfHandleSize, handlePos.y - halfHandleSize),
            ImVec2(handlePos.x + halfHandleSize, handlePos.y + halfHandleSize),
            IM_COL32(20, 30, 35, 255),
            1.5f);

        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            gameDesc.sceneFocused = false;
            Vec2 delta = GetRectTransformDragDelta(rectTransform, camera, viewport);
            if (delta.LengthSquared() > 0.f)
            {
                Vec2 offsetMin = rectTransform->GetOffsetMin();
                Vec2 offsetMax = rectTransform->GetOffsetMax();

                switch (handleTypes[i])
                {
                case RectTransformHandleType::Move:
                    offsetMin += delta;
                    offsetMax += delta;
                    break;
                case RectTransformHandleType::Left:
                    offsetMin.x += delta.x;
                    break;
                case RectTransformHandleType::Right:
                    offsetMax.x += delta.x;
                    break;
                case RectTransformHandleType::Bottom:
                    offsetMin.y += delta.y;
                    break;
                case RectTransformHandleType::Top:
                    offsetMax.y += delta.y;
                    break;
                }

                rectTransform->SetOffsets(offsetMin, offsetMax);
            }
        }
        ImGui::PopID();
    }
    ImGui::PopID();
}

Vec2 SceneView::GetRectTransformDragDelta(RectTransform* rectTransform, Camera* camera, Viewport& viewport) const
{
    if (rectTransform == nullptr || camera == nullptr)
        return Vec2::Zero;

    ImGuiIO& io = ImGui::GetIO();
    if (io.MouseDelta.x == 0.f && io.MouseDelta.y == 0.f)
        return Vec2::Zero;

    Matrix view = camera->GetViewMatrix();
    Matrix projection = camera->GetProjectionMatrix();
    Matrix identity = Matrix::Identity;

    Vec3 pivotPosition = viewport.Project(Vec3::Zero, rectTransform->GetWorldMatrix(), view, projection);
    ImVec2 previousMouse(io.MousePos.x - io.MouseDelta.x, io.MousePos.y - io.MouseDelta.y);
    Vec3 previousWorld = viewport.Unproject(Vec3(previousMouse.x, previousMouse.y, pivotPosition.z), identity, view, projection);
    Vec3 currentWorld = viewport.Unproject(Vec3(io.MousePos.x, io.MousePos.y, pivotPosition.z), identity, view, projection);
    Vec3 worldDelta = currentWorld - previousWorld;

    Transform* parent = rectTransform->GetParent();
    if (parent == nullptr)
        return Vec2(worldDelta.x, worldDelta.y);

    Matrix parentWorldInv = parent->GetWorldMatrix().Invert();
    Vec3 parentLocalDelta = Vec3::TransformNormal(worldDelta, parentWorldInv);
    Vec3 parentScale = parent->GetScale();
    return Vec2(parentLocalDelta.x * parentScale.x, parentLocalDelta.y * parentScale.y);
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