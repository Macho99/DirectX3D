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

    _sceneView = make_unique<SceneView>();
    _hierarchy = make_unique<Hierarchy>();
    _console = make_unique<Console>();
    _inspector = make_unique<Inspector>();
    _debugTexWindows.push_back(make_unique<DebugTexWindow>(L"ShadowMap0", GRAPHICS->GetShadowMap(0)));
    _debugTexWindows.push_back(make_unique<DebugTexWindow>(L"ShadowMap1", GRAPHICS->GetShadowMap(1)));
    _debugTexWindows.push_back(make_unique<DebugTexWindow>(L"ShadowMap2", GRAPHICS->GetShadowMap(2)));
    _debugTexWindows.push_back(make_unique<DebugTexWindow>(L"NormalDepthMap", GRAPHICS->GetNormalDepthMap()));
    _debugTexWindows.push_back(make_unique<DebugTexWindow>(L"SsaoMap", GRAPHICS->GetSsaoMap()));
    _debugTexWindows.push_back(make_unique<DebugTexWindow>(L"PostProcess", GRAPHICS->GetPostProcessDebugTexture(0)));
}

void EditorManager::Update()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Window"))
        {
            ImGui::MenuItem("Hierarchy", nullptr, &_hierarchy->IsOpen);
            ImGui::MenuItem("Scene", nullptr, & _sceneView->IsOpen);
            ImGui::MenuItem("Inspector", nullptr, &_inspector->IsOpen);
            ImGui::MenuItem("Console", nullptr, &_console->IsOpen);

            for (unique_ptr<DebugTexWindow>& debugTexWindow : _debugTexWindows)
            {
                wstring name = debugTexWindow->GetName();
                ImGui::MenuItem(string(name.begin(), name.end()).c_str(), nullptr, &debugTexWindow->IsOpen);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
    DrawDockSpace();
    bool showDemo = true;
    ImGui::ShowDemoWindow(&showDemo);

    _hierarchy->CheckAndOnGUI();
    _sceneView->CheckAndOnGUI();
    _console->CheckAndOnGUI();
    _inspector->CheckAndOnGUI();
    for (unique_ptr<DebugTexWindow>& debugTexWindow : _debugTexWindows)
    {
        debugTexWindow->CheckAndOnGUI();
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

    _sceneView.reset();
    _hierarchy.reset();
    _inspector.reset();
    _console.reset();
    _debugTexWindows.clear();
}
