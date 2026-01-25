#include "pch.h"
#include "ImGuiManager.h"


void ImGuiManager::Init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(GAME->GetGameDesc().hWnd);
	ImGui_ImplDX11_Init(DEVICE.Get(), DC.Get());
}

void ImGuiManager::Update()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

    DrawDockSpace();    
    bool showDemo = true;
    ImGui::ShowDemoWindow(&showDemo);
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
}

void ImGuiManager::DrawDockSpace()
{
    static bool dockspaceOpen = true;
    static bool opt_fullscreen = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoDocking;

    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        window_flags |=
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("DockSpaceRoot", &dockspaceOpen, window_flags);

    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(
        dockspace_id,
        ImVec2(0.0f, 0.0f),
        dockspace_flags
    );

    ImGui::End();
}

void ImGuiManager::Render()
{
	// Rendering
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiManager::OnDestroy()
{    // Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
