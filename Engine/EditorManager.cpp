#include "pch.h"
#include "EditorManager.h"
#include "SceneView.h"
#include "Hierarchy.h"
#include "Console.h"
#include "Inspector.h"
#include "DebugTexWindow.h"
#include "ContentBrowser.h"

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

    ImFont* font = io.Fonts->AddFontFromFileTTF(
        "..\\Assets\\Pretendard-Medium.ttf",
        18.0f,
        NULL,
        io.Fonts->GetGlyphRangesKorean()
    );

    IM_ASSERT(font != NULL);
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
    _editorWindows.push_back(make_unique<ContentBrowser>());
    _editorWindows.push_back(make_unique<DebugTexWindow>("ShadowMap0", []() { return GRAPHICS->GetShadowMap(0).Resolve(); }));
    _editorWindows.push_back(make_unique<DebugTexWindow>("ShadowMap1", []() { return GRAPHICS->GetShadowMap(1).Resolve(); }));
    _editorWindows.push_back(make_unique<DebugTexWindow>("ShadowMap2", []() { return GRAPHICS->GetShadowMap(2).Resolve(); }));
    _editorWindows.push_back(make_unique<DebugTexWindow>("NormalDepthMap", []() { return GRAPHICS->GetNormalDepthMap().Resolve(); }));
    _editorWindows.push_back(make_unique<DebugTexWindow>("SsaoMap", []() { return GRAPHICS->GetSsaoMap().Resolve(); }));
    _editorWindows.push_back(make_unique<DebugTexWindow>("PostProcess", []() { return GRAPHICS->GetPostProcessDebugTexture(0).Resolve(); }));

    for (unique_ptr<EditorWindow>& editorWindow : _editorWindows)
    {
        editorWindow->Init(this);
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
                if (editorWindow->IsMenuEnabled() == false)
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
        editorWindow->Update();
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
{
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    _editorWindows.clear();
}

void EditorManager::GetInspectorRef(OUT TransformRef& transformRef, OUT AssetRef& assetRef, OUT int& subAssetIndex) const
{
    transformRef = _inspectorTransform;
    assetRef = _inspectorAsset;
    subAssetIndex = _inspectorSubAsset;
}

void EditorManager::ClickTransform(const TransformRef& transformRef)
{
    if (_inspectorLock == false)
    {
        _inspectorAsset = AssetRef();
        _inspectorSubAsset = -1;

        _inspectorTransform = transformRef;
    }
    _hierarchyTransform = transformRef;
}

void EditorManager::ClickAsset(const AssetRef& assetRef)
{
    AssetId ownerAssetId;
    int subAssetIndex;
    RESOURCES->GetAssetDatabase().GetOwnerAssetId(assetRef.GetAssetId(), OUT ownerAssetId, OUT subAssetIndex);

    if (_inspectorLock == false)
    {
        _inspectorTransform = TransformRef();
        _inspectorAsset = ownerAssetId;
        _inspectorSubAsset = subAssetIndex;
    }

    _contentBrowserAsset = ownerAssetId;
    _contentBrowserSubAsset = subAssetIndex;
}

bool EditorManager::TryGetHierarchyTransform(OUT TransformRef& transformRef) const
{
    if (_hierarchyTransform.IsValid())
    {
        transformRef = _hierarchyTransform;
        return true;
    }
    transformRef = TransformRef();
    return false;
}

void EditorManager::FocusHierarchyTransform(const TransformRef& transformRef)
{
    _hierarchyTransform = transformRef;
}

bool EditorManager::TryGetContentBrowserAsset(OUT AssetRef& assetRef, OUT int& subAssetIndex) const
{
    if (_contentBrowserAsset.IsValid())
    {
        assetRef = _contentBrowserAsset;
        subAssetIndex = _contentBrowserSubAsset;
        return true;
    }
    assetRef = AssetRef();
    subAssetIndex = -1;
    return false;
}

void EditorManager::FocusContentBrowserAsset(const AssetRef& assetRef)
{
    AssetId ownerAssetId;
    int subAssetIndex;
    RESOURCES->GetAssetDatabase().GetOwnerAssetId(assetRef.GetAssetId(), OUT ownerAssetId, OUT subAssetIndex);

    _contentBrowserAsset = ownerAssetId;
    _contentBrowserSubAsset = subAssetIndex;
}

Texture* EditorManager::GetEditorIconTexture(EditorIcon icon)
{
    switch (icon)
    {
    case EditorIcon::Lock:
        return RESOURCES->GetEditorTexture("Icon_Lock", L"..\\EditorResource\\Lock.png");
    case EditorIcon::Unlock:
        return RESOURCES->GetEditorTexture("Icon_Unlock", L"..\\EditorResource\\Unlock.png");
    }

    ASSERT(false, "EditorManager::GetEditorIconTexture: Unsupported icon type: " + std::to_string((int)icon));
    return nullptr;
}
