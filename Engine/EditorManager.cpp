#include "pch.h"
#include "EditorManager.h"
#include "SceneView.h"
#include "Hierarchy.h"
#include "Console.h"
#include "Inspector.h"
#include "DebugTexWindow.h"

EditorManager::EditorManager()
{
}

EditorManager::~EditorManager()
{

}

void EditorManager::Init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();// (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(GAME->GetGameDesc().hWnd);
	ImGui_ImplDX11_Init(DEVICE.Get(), DC.Get());

    _editorWindows.push_back(make_unique<SceneView>());
    _editorWindows.push_back(make_unique<Hierarchy>());
    _editorWindows.push_back(make_unique<Console>());
    _editorWindows.push_back(make_unique<Inspector>());
    _editorWindows.push_back(make_unique<DebugTexWindow>("ShadowMap0", []() { return GRAPHICS->GetShadowMap(0).get(); }));
    _editorWindows.push_back(make_unique<DebugTexWindow>("ShadowMap1", []() { return GRAPHICS->GetShadowMap(1).get(); }));
    _editorWindows.push_back(make_unique<DebugTexWindow>("ShadowMap2", []() { return GRAPHICS->GetShadowMap(2).get(); }));
    _editorWindows.push_back(make_unique<DebugTexWindow>("NormalDepthMap", []() { return GRAPHICS->GetNormalDepthMap().get(); }));
    _editorWindows.push_back(make_unique<DebugTexWindow>("SsaoMap", []() { return GRAPHICS->GetSsaoMap().get(); }));
    _editorWindows.push_back(make_unique<DebugTexWindow>("PostProcess", [](){ return GRAPHICS->GetPostProcessDebugTexture(0).get();}));

    for (unique_ptr<EditorWindow>& editorWindow : _editorWindows)
    {
        editorWindow->Init();
    }
}

void EditorManager::Update()
{
    ImGuiIO& io = ImGui::GetIO();
    if (INPUT->GetButton(KEY_TYPE::LBUTTON) || INPUT->GetButton(KEY_TYPE::RBUTTON))
    {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    }
    else if (INPUT->GetButtonUp(KEY_TYPE::LBUTTON) || INPUT->GetButtonUp(KEY_TYPE::RBUTTON))
    {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    }
    else
    {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    }



	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Window"))
        {
            for (unique_ptr<EditorWindow>& editorWindow : _editorWindows)
            {
                if(editorWindow->IsMenuEnabled() == false)
                    continue;

                ImGui::MenuItem(editorWindow->GetName().c_str(), nullptr, &editorWindow->IsOpen);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
    DrawDockSpace();
    bool showDemo = true;
    ImGui::ShowDemoWindow(&showDemo);

    for (unique_ptr<EditorWindow>& editorWindow : _editorWindows)
    {
        editorWindow->CheckAndOnGUI();
    }
}

void EditorManager::DrawDockSpace()
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

void EditorManager::Render()
{
	// Rendering
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void EditorManager::OnDestroy()
{    // Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

    _editorWindows.clear();
}
