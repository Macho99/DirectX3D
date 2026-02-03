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

    ImGui::Begin(_windowName.c_str(), &IsOpen);

    // 창의 콘텐츠 영역 크기 (스크롤/패딩 제외)
    ImVec2 avail = ImGui::GetContentRegionAvail();
    UINT w = (UINT)max(10.0f, avail.x);
    UINT h = (UINT)max(10.0f, avail.y);

    GAME->GetGameDesc().sceneWidth = w;
    GAME->GetGameDesc().sceneHeight = h;

    ImGui::Image((ImTextureID)GRAPHICS->GetSceneViewSRV().Get(), avail);

    ImGui::End();
}
