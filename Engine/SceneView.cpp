#include "pch.h"
#include "SceneView.h"

SceneView::SceneView()
    :Super("Scene")
{
}

SceneView::~SceneView()
{
}

void SceneView::Init()
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
}
