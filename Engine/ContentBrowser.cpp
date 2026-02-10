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
            if (e.absPath.parent_path() == _currentFolder || e.oldAbsPath.parent_path() == _currentFolder)
                needRefresh = true;
        });
    SetCurrentFolder(_root);
}

void ContentBrowser::Update()
{
}

void ContentBrowser::RefreshBrowserItems()
{
    _curBrowserItems.clear();

    for (auto& de : fs::directory_iterator(_currentFolder))
    {
        fs::path path = de.path();
        if (MetaStore::IsMetaFile((path)))
            continue;

        Guid guid;
        if (RESOURCES->TryGetGuidByPath(path, guid) == false)
        {
            DBG->LogW(L"[ContentBrowser] Refresh: Skip non-asset file: " + path.wstring());
            continue;
        }

        BrowserItem browserItem;
        browserItem.absPath = path;
        browserItem.guid = guid;
        browserItem.isFolder = fs::is_directory(path);
        _curBrowserItems.push_back((browserItem));
    }

    // 3) 정렬: 폴더 먼저, 그 다음 이름순
    std::sort(_curBrowserItems.begin(), _curBrowserItems.end(), [](const BrowserItem& a, const BrowserItem& b)
        {
            if (a.isFolder != b.isFolder) return a.isFolder > b.isFolder;
            return a.absPath.filename().wstring() < b.absPath.filename().wstring();
        });
}

void ContentBrowser::OnGUI()
{
    Super::OnGUI();

    if (needRefresh)
    {
        RefreshBrowserItems();
        needRefresh = false;
    }

    ImGui::Columns(2, nullptr, true);
    ImGui::SetColumnWidth(0, _leftPaneWidth);

    DrawLeftFolderTree();

    ImGui::NextColumn();
    DrawRightUnityStyle();

    ImGui::Columns(1);
}

void ContentBrowser::DrawLeftFolderTree()
{
    ImGui::BeginChild("CB_Left", ImVec2(0, 0), true);

    // 루트는 항상 펼친 상태로 시작
    ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen;
    bool open = ImGui::TreeNodeEx(_root.wstring().c_str(), rootFlags, "%ls", DisplayName(_root).c_str());
    if (ImGui::IsItemClicked())
        SetCurrentFolder(_root);

    if (open)
    {
        DrawFolderNode(_root);
        ImGui::TreePop();
    }

    ImGui::EndChild();
}

void ContentBrowser::DrawFolderNode(const fs::path& folderAbs)
{
    // 현재 폴더의 하위 폴더만 나열
    std::vector<fs::path> dirs;
    try
    {
        for (auto& de : fs::directory_iterator(folderAbs))
        {
            if (de.is_directory())
                dirs.push_back(de.path());
        }
    }
    catch (...) {}

    std::sort(dirs.begin(), dirs.end(), [](const fs::path& a, const fs::path& b)
        {
            return a.filename().wstring() < b.filename().wstring();
        });

    for (auto& dir : dirs)
    {
        const bool isSelected = (fs::equivalent(dir, _currentFolder));
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanFullWidth |
            (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

        // leaf 여부를 정확히 판단하려면 한번 더 디렉토리 확인이 필요하지만,
        // 지금 단계에선 간단하게 그냥 트리노드로 둡니다.
        bool open = ImGui::TreeNodeEx(dir.wstring().c_str(), flags, "%ls", DisplayName(dir).c_str());

        if (ImGui::IsItemClicked())
            SetCurrentFolder(dir);

        if (open)
        {
            DrawFolderNode(dir);
            ImGui::TreePop();
        }
    }
}

void ContentBrowser::DrawRightUnityStyle()
{
    ImGui::BeginChild("CB_Right_Unity", ImVec2(0, 0), true);

    DrawToolbarRow();
    ImGui::Separator();

    // 검색 필터 적용
    std::vector<BrowserItem> filtered;
    filtered.reserve(_curBrowserItems.size());
    for (auto& it : _curBrowserItems)
    {
        if (PassSearchFilter(it))
            filtered.push_back(it);
    }

    // 보기 모드
    if (_viewMode == ViewMode::Grid)
        DrawItemsGrid(filtered);
    else
        DrawItemsList(filtered);

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

void ContentBrowser::DrawItemsGrid(const std::vector<BrowserItem>& items)
{
    // 유니티 Project 창처럼: 그리드 타일(아이콘 + 파일명)
    ImGuiStyle& style = ImGui::GetStyle();

    float cellSize = _thumbSize + _thumbPad * 2.0f;
    float availX = ImGui::GetContentRegionAvail().x;
    int columns = (int)(availX / cellSize);
    if (columns < 1) columns = 1;

    int col = 0;
    for (const auto& it : items)
    {
        ImGui::PushID(it.absPath.wstring().c_str());

        if (col > 0) ImGui::SameLine();

        ImGui::BeginGroup();

        // 아이콘 영역(버튼/셀렉터 역할)
        ImVec2 iconSize(_thumbSize, _thumbSize);

        // 폴더/파일 아이콘을 다르게 보여주고 싶으면 여기서 DrawList/색상/텍스처 썸네일로 교체
        // 지금은 간단히 폴더는 "📁", 파일은 "■"
        const char* icon = it.isFolder ? "DIR" : "FILE";

        bool selected = (_selectedPath == it.absPath);

        // 선택된 것처럼 보이게: 배경
        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonHovered]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_ButtonHovered]);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_ButtonHovered]);
        }

        if (ImGui::Button(icon, ImVec2(iconSize.x, iconSize.y)))
        {
            _selectedPath = it.absPath;
        }

        if (selected)
        {
            ImGui::PopStyleColor(3);
        }

        // 더블클릭 폴더 들어가기
        if (it.isFolder && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            SetCurrentFolder(it.absPath);
            _selectedPath.clear();
        }

        // 이름(두 줄까지 표시 느낌)
        std::wstring nameW = DisplayName(it.absPath);
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

void ContentBrowser::DrawItemsList(const std::vector<BrowserItem>& items)
{
    // 유니티 Project의 List 모드 느낌: 한 줄씩, 왼쪽 작은 아이콘 + 이름
    for (const auto& it : items)
    {
        ImGui::PushID(it.absPath.wstring().c_str());

        bool selected = (_selectedPath == it.absPath);

        // 작은 아이콘(텍스트로 대체)
        ImGui::TextUnformatted(it.isFolder ? "[D]" : "[F]");
        ImGui::SameLine();

        std::wstring nameW = DisplayName(it.absPath);
        std::string name = Utils::ToString(nameW);

        if (ImGui::Selectable(name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
        {
            _selectedPath = it.absPath;
        }

        if (it.isFolder && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            SetCurrentFolder(it.absPath);
            _selectedPath.clear();
        }

        ImGui::PopID();
    }
}

bool ContentBrowser::PassSearchFilter(const BrowserItem& it) const
{
    if (_search.empty()) return true;

    std::wstring nameW = DisplayName(it.absPath);
    std::wstring needleW = Utils::ToWString(_search);

    // 대소문자 무시(간단)
    auto lower = [](std::wstring s)
        {
            for (auto& ch : s) ch = (wchar_t)towlower(ch);
            return s;
        };

    std::wstring nameL = lower(nameW);
    std::wstring needleL = lower(needleW);
    return nameL.find(needleL) != std::wstring::npos;
}

wstring ContentBrowser::DisplayName(const fs::path& p)
{
    auto name = p.filename().wstring();
    if (name.empty()) return p.wstring();
    return name;
}

void ContentBrowser::SetCurrentFolder(const fs::path& folderAbs)
{
    if (fs::exists(folderAbs) && fs::is_directory(folderAbs))
    {
        _currentFolder = folderAbs;
        needRefresh = true;
    }
}
