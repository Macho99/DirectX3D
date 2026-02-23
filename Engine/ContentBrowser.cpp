#include "pch.h"
#include "ContentBrowser.h"
#include "Utils.h"

#include "MetaFile.h"
#include "MetaStore.h"

ContentBrowser::ContentBrowser()
    : Super("ContentBrower")
{
}

ContentBrowser::~ContentBrowser()
{
    RESOURCES->SetOnFileEventCallback(nullptr);
}


void ContentBrowser::Init(EditorManager* editorManager)
{
    Super::Init(editorManager);

    _root = RESOURCES->GetRootPath();
    RESOURCES->SetOnFileEventCallback([this](const FsEvent& e)
        {
            if (fs::is_directory(e.absPath))
            {
                _tree.Invalidate(e.absPath);
                _tree.Invalidate(e.absPath.parent_path());
            }
        });
    SetCurrentFolder(_root);
    _tree.SetRoot(_root);
}

void ContentBrowser::Update()
{
}

void ContentBrowser::GetCurMetaFiles()
{
    _curMetaFiles.clear();
    for (auto& de : fs::directory_iterator(_currentFolder))
    {
        fs::path path = de.path();
        if (MetaStore::IsMetaFile((path)))
            continue;

        MetaFile* meta = nullptr;
        if (RESOURCES->TryGetMetaByPath(path, meta) == false)
        {
            DBG->LogW(L"[ContentBrowser] Refresh: Skip non-asset file: " + path.wstring());
            continue;
        }
        _curMetaFiles.push_back(meta);
    }

    // 3) 정렬: 폴더 먼저, 그 다음 이름순
    std::sort(_curMetaFiles.begin(), _curMetaFiles.end(), [](const MetaFile* lhs, const MetaFile* rhs)
        {
            if (lhs->GetResourceType() == ResourceType::Folder && rhs->GetResourceType() != ResourceType::Folder)
                return true;
            if (lhs->GetResourceType() != ResourceType::Folder && rhs->GetResourceType() == ResourceType::Folder)
                return false;

            return lhs->GetAbsPath().filename().wstring() < rhs->GetAbsPath().filename().wstring();
        });
}

void ContentBrowser::OnGUI()
{
    Super::OnGUI();

    if (!fs::exists(_currentFolder))
    {
        _currentFolder = _root;
    }
    GetCurMetaFiles();

    ImGui::Columns(2, nullptr, true);
    ImGui::SetColumnWidth(0, _leftPaneWidth);

    DrawLeftFolderTree();

    ImGui::NextColumn();
    DrawRightUnityStyle();

    ImGui::Columns(1);

    _curMetaFiles.clear();
}

void ContentBrowser::DrawLeftFolderTree()
{
    ImGui::BeginChild("CB_Left", ImVec2(0, 0), true);

    auto* rootNode = _tree.GetRoot();
    if (!rootNode)
    {
        ImGui::EndChild();
        return;
    }

    // 루트는 항상 펼침
    ImGuiTreeNodeFlags rootFlags =
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanFullWidth;

    const bool rootSelected = SafeEquivalent(rootNode->abs, _currentFolder);
    if (rootSelected) rootFlags |= ImGuiTreeNodeFlags_Selected;

    // ID는 포인터로 안정적으로
    bool open = ImGui::TreeNodeEx((void*)rootNode, rootFlags, "%s", rootNode->displayName.c_str());
    if (ImGui::IsItemClicked())
        SetCurrentFolder(rootNode->abs);

    if (open)
    {
        _tree.EnsureScanned(rootNode);
        DrawFolderNodeRecursive(rootNode);
        ImGui::TreePop();
    }

    ImGui::EndChild();
}

void ContentBrowser::DrawFolderNodeRecursive(FolderTreeCache::Node* node)
{
    // 같은 레벨 자식이 엄청 많을 때만 의미가 있으니, 클리퍼 적용
    ImGuiListClipper clipper;
    clipper.Begin((int)node->children.size());

    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
        {
            auto* child = node->children[i];

            const bool isSelected = SafeEquivalent(child->abs, _currentFolder);

            // 자식 존재 여부를 빠르게 leaf 플래그로 반영하려면
            // "열릴 때" 스캔하기 전까지는 leaf로 두지 않는 편이 안전합니다.
            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanFullWidth |
                (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

            _tree.EnsureScanned(child);
            // 이미 스캔된 노드인데 자식이 없으면 leaf로 최적화(화살표 제거)
            if (child->scanned && !child->hasChildren)
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

            bool open = ImGui::TreeNodeEx((void*)child, flags, "%s", child->displayName.c_str());

            if (ImGui::IsItemClicked())
                SetCurrentFolder(child->abs);

            if (open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
            {
                DrawFolderNodeRecursive(child);
                ImGui::TreePop();
            }
            else if (open && (flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
            {
                // leaf라서 TreePop 불필요
            }
        }
    }
}

void ContentBrowser::DrawRightUnityStyle()
{
    ImGui::BeginChild("CB_Right_Unity", ImVec2(0, 0), true);

    DrawToolbarRow();
    ImGui::Separator();

    // 보기 모드
    if (_viewMode == ViewMode::Grid)
        DrawItemsGrid();
    else
        DrawItemsList();

    ImGui::EndChild();
}

void ContentBrowser::DrawToolbarRow()
{
    // 상단 한 줄: Breadcrumb(왼쪽) + Search(오른쪽) + View toggle
    DrawBreadcrumb();

    // 오른쪽 정렬을 위해 같은 라인에 더미 공간
    ImGui::SameLine();
    float rightWidth = 260.0f; // Search+토글 자리
    float avail = ImGui::GetContentRegionAvail().x;
    if (avail > rightWidth)
        ImGui::Dummy(ImVec2(avail - rightWidth, 0));

    ImGui::SameLine();
    DrawSearchBox();

    ImGui::SameLine();
    DrawViewToggle();
}

void ContentBrowser::DrawBreadcrumb()
{
    // 유니티 느낌: 경로를 버튼들로 쪼개기
    // root부터 currentFolder까지의 조상 목록 만들기
    std::vector<fs::path> parts;
    {
        fs::path p = _currentFolder;
        while (!p.empty())
        {
            parts.push_back(p);
            if (p == _root) break;
            p = p.parent_path();
        }
        std::reverse(parts.begin(), parts.end());
        if (parts.empty()) parts.push_back(_root);
    }

    // "Root > Sub > Sub2" 형태 버튼
    for (size_t i = 0; i < parts.size(); ++i)
    {
        fs::path p = parts[i];
        std::wstring labelW = (p == _root) ? DisplayName(_root) : DisplayName(p);
        std::string label = Utils::ToString(labelW);

        if (ImGui::SmallButton(label.c_str()))
        {
            SetCurrentFolder(p);
            _selectedPath.clear();
        }

        if (i + 1 < parts.size())
        {
            ImGui::SameLine();
            ImGui::TextUnformatted(">");
            ImGui::SameLine();
        }
    }
}

void ContentBrowser::DrawSearchBox()
{
    ImGui::SetNextItemWidth(160.0f);
    //ImGui::InputTextWithHint("##CB_Search", "Search", &_search);
}

void ContentBrowser::DrawViewToggle()
{    // 유니티 느낌: Grid/List 토글 + 아이콘 크기 슬라이더
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

    bool isGrid = (_viewMode == ViewMode::Grid);
    if (ImGui::SmallButton(isGrid ? "Grid*" : "Grid"))
        _viewMode = ViewMode::Grid;

    ImGui::SameLine();
    bool isList = (_viewMode == ViewMode::List);
    if (ImGui::SmallButton(isList ? "List*" : "List"))
        _viewMode = ViewMode::List;

    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::SliderFloat("##CB_Thumb", &_thumbSize, 48.0f, 120.0f, "");

    ImGui::PopStyleVar();
}

void ContentBrowser::DrawItemsGrid()
{
    // 유니티 Project 창처럼: 그리드 타일(아이콘 + 파일명)
    ImGuiStyle& style = ImGui::GetStyle();

    float cellSize = _thumbSize + _thumbPad * 2.0f;
    float availX = ImGui::GetContentRegionAvail().x;
    int columns = (int)(availX / cellSize);
    if (columns < 1) columns = 1;

    int col = 0;
    for (const auto meta : _curMetaFiles)
    {
        fs::path absPath = meta->GetAbsPath();
        bool isFolder = (meta->GetResourceType() == ResourceType::Folder);
        ImGui::PushID(absPath.wstring().c_str());

        if (col > 0) ImGui::SameLine();

        ImGui::BeginGroup();

        // 아이콘 영역(버튼/셀렉터 역할)
        ImVec2 iconSize(_thumbSize, _thumbSize);

        // 폴더/파일 아이콘을 다르게 보여주고 싶으면 여기서 DrawList/색상/텍스처 썸네일로 교체
        // 지금은 간단히 폴더는 "📁", 파일은 "■"
        const char* icon = isFolder ? "DIR" : "FILE";

        bool selected = (_selectedPath == absPath);

        // 선택된 것처럼 보이게: 배경
        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonHovered]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_ButtonHovered]);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_ButtonHovered]);
        }

        if (ImGui::Button(icon, ImVec2(iconSize.x, iconSize.y)))
        {
            _selectedPath = absPath;
        }

        if (selected)
        {
            ImGui::PopStyleColor(3);
        }

        // 더블클릭 폴더 들어가기
        if (isFolder && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            SetCurrentFolder(absPath);
            _selectedPath.clear();
        }

        // 이름(두 줄까지 표시 느낌)
        std::wstring nameW = DisplayName(absPath);
        std::string name = Utils::ToString(nameW);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + _thumbSize);
        ImGui::TextUnformatted(name.c_str());
        ImGui::PopTextWrapPos();

        ImGui::EndGroup();
        ImGui::PopID();

        col++;
        if (col >= columns)
        {
            col = 0;
        }
    }
}

void ContentBrowser::DrawItemsList()
{
    // 유니티 Project의 List 모드 느낌: 한 줄씩, 왼쪽 작은 아이콘 + 이름
    for (const auto meta : _curMetaFiles)
    {
        fs::path absPath = meta->GetAbsPath();
        ImGui::PushID(absPath.c_str());

        bool selected = (_selectedPath == absPath);

        bool isFolder = meta->GetResourceType() == ResourceType::Folder;
        // 작은 아이콘(텍스트로 대체)
        ImGui::TextUnformatted(isFolder ? "[D]" : "[F]");
        ImGui::SameLine();

        std::wstring nameW = DisplayName(absPath);
        std::string name = Utils::ToString(nameW);

        if (ImGui::Selectable(name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
        {
            _selectedPath = absPath;
        }

        if (isFolder && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            SetCurrentFolder(absPath);
            _selectedPath.clear();
        }

        ImGui::PopID();
    }
}

wstring ContentBrowser::DisplayName(const fs::path& p)
{
    return p.filename().wstring();
}

void ContentBrowser::SetCurrentFolder(const fs::path& folderAbs)
{
    if (fs::exists(folderAbs) && fs::is_directory(folderAbs))
    {
        _currentFolder = folderAbs;
    }
}

bool ContentBrowser::SafeEquivalent(const fs::path& a, const fs::path& b)
{
    // fs::equivalent는 존재하지 않는 경로에 대해 예외를 던지므로, 안전하게 비교하는 헬퍼
    std::error_code ec;
    bool eq = fs::equivalent(a, b, ec);
    if (ec)
    {
        // 예외 대신 false 반환(존재하지 않거나 접근 불가한 경우)
        return false;
    }
    return eq;
}
