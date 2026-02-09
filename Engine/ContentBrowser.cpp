#include "pch.h"
#include "ContentBrowser.h"
#include "Utils.h"

#include "MetaFile.h"
#include "MetaStore.h"

ContentBrowser::ContentBrowser()
    : Super("ContentBrower"),
    assetDatabase()
{
    assetDatabase.AddListener([this](const AssetEvent& e) { OnAssetEvent(e); });
}

ContentBrowser::~ContentBrowser()
{
    watcher.Stop();
}

wstring ContentBrowser::ToStr(FsAction fsAction)
{
    switch (fsAction)
    {
        case FsAction::Added: return L"Added";
        case FsAction::Removed: return L"Removed";
        case FsAction::Modified: return L"Modified";
        case FsAction::Renamed: return L"Renamed";
        default: return L"?";
    }
}

void ContentBrowser::Init(EditorManager* editorManager)
{
    Super::Init(editorManager);

    _root = L"..\\Resources";
    _currentFolder = _root;

    if (!watcher.Start(_root, true, [&](const FsEvent& e)
        {
            // 워처 스레드: 절대 여기서 무거운 일 하지 말고 Push만!
            eventThreadQueue.Push(e);
        }))
    {
        DBG->LogW(L"Watcher start failed");
    }
    else
    {
        DBG->LogW(L"Watching: " + _root.wstring());
    }
}

void ContentBrowser::Update()
{
    pendingEvents.clear();
    eventThreadQueue.PopAll(pendingEvents);

    // 1) 필터 + 디바운서 입력
    for (auto& e : pendingEvents)
    {
        if (!IsInterestingFile(e.absPath))
            continue;

        debouncer.Push(e);
    }

    // 2) 디바운스 완료분 배출 (예: 300ms)
    readyEvents.clear();
    debouncer.PopReady(300, readyEvents);

    // 3) 최종 처리(여기서부터 메인 스레드)
    for (auto& e : readyEvents)
    {
        assetDatabase.OnFileEvent(e);
    }
}

void ContentBrowser::OnGUI()
{
    Super::OnGUI();

    ImGui::Columns(2, nullptr, true);
    ImGui::SetColumnWidth(0, _leftPaneWidth);

    DrawLeftFolderTree();

    ImGui::NextColumn();
    DrawRightUnityStyle();

    ImGui::Columns(1);
}

bool ContentBrowser::IsInterestingFile(const fs::path& p)
{
    if (!p.has_extension()) return false;

    auto ext = p.extension().wstring();
    for (auto& ch : ext) ch = (wchar_t)towlower(ch);

    // 일단 임포트 관련 핵심만
    if (ext == L".fbx") return true;
    if (ext == L".png" || ext == L".tga" || ext == L".jpg" || ext == L".jpeg") return true;

    // meta는 다음 단계에서 다시 포함시킬 겁니다.
    // if (ext == L".meta") return true;

    return false;
}

void ContentBrowser::OnAssetEvent(const AssetEvent& e)
{
    fs::path parent = ParentFolderOfEvent(e);
    if (parent.empty()) return;

    _dirtyFolders.insert(parent.wstring());

    DBG->LogW(L"[Browser] Dirty folder: " + parent.wstring());
}

vector<BrowserItem> ContentBrowser::GetItemsForFolder(const fs::path& folderAbs)
{
    std::vector<BrowserItem> items;

    // 1) 폴더(디스크 스캔)
    //   - 숨김/권한 예외는 다음 단계에서 다듬어도 됩니다.
    try
    {
        for (auto& de : fs::directory_iterator(folderAbs))
        {
            if (!de.is_directory()) continue;

            BrowserItem it;
            it.absPath = de.path();
            it.guid = Guid();     // 폴더는 guid 없음
            it.isFolder = true;
            items.push_back(std::move(it));
        }
    }
    catch (...) {}

    // 2) 파일(AssetDB 인덱스에서)
    auto files = assetDatabase.ListChildrenFiles(folderAbs);
    for (auto& p : files)
    {
        Guid g;
        assetDatabase.TryGetGuidByPath(p, g);

        BrowserItem it;
        it.absPath = p;
        it.guid = g;
        it.isFolder = false;
        items.push_back(std::move(it));
    }

    // 3) 정렬: 폴더 먼저, 그 다음 이름순
    std::sort(items.begin(), items.end(), [](const BrowserItem& a, const BrowserItem& b)
        {
            if (a.isFolder != b.isFolder) return a.isFolder > b.isFolder;
            return a.absPath.filename().wstring() < b.absPath.filename().wstring();
        });

    RefreshFolder(folderAbs);
    return items;
}

bool ContentBrowser::IsFolderDirty(const fs::path& folderAbs) const
{
    return _dirtyFolders.find(folderAbs.wstring()) != _dirtyFolders.end();
}

void ContentBrowser::RefreshFolder(const fs::path& folderAbs)
{
    _dirtyFolders.erase(folderAbs.wstring());
    DBG->LogW(L"[Browser] Refresh folder: " + folderAbs.wstring());
}

fs::path ContentBrowser::ParentFolderOfEvent(const AssetEvent& e)
{
    return e.fs.absPath.parent_path();
}

void ContentBrowser::DrawLeftFolderTree()
{
    ImGui::BeginChild("CB_Left", ImVec2(0, 0), true);

    // 루트는 항상 펼친 상태로 시작
    ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen;
    bool open = ImGui::TreeNodeEx(_root.wstring().c_str(), rootFlags, "%ls", DisplayName(_root).c_str());
    if (ImGui::IsItemClicked())
        _currentFolder = _root;

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
            _currentFolder = dir;

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

    // 목록 가져오기 (model이 정렬/refresh)
    auto items = GetItemsForFolder(_currentFolder);

    // 검색 필터 적용
    std::vector<BrowserItem> filtered;
    filtered.reserve(items.size());
    for (auto& it : items)
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
            _currentFolder = p;
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
            _currentFolder = it.absPath;
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
            _currentFolder = it.absPath;
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
