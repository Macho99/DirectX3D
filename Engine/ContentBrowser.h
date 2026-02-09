#pragma once
#include "EditorWindow.h"
#include "DirectoryWatcherWin.h"
#include "ThreadSafeQueue.h"
#include "FsEventDebouncer.h"
#include "AssetDatabase.h"

struct BrowserItem
{
    fs::path absPath;
    Guid guid;       // 파일이면 guid, 폴더면 invalid
    bool isFolder = false;
};
enum class ViewMode
{
    Grid,
    List
};

class ContentBrowser : public EditorWindow
{
    using Super = EditorWindow;
public:
    ContentBrowser();
    ~ContentBrowser();

    void Init(EditorManager* editorManager) override;
    void Update() override;

    // UI가 “현재 보고 있는 폴더”의 아이템 목록을 요청할 때 사용
    vector<BrowserItem> GetItemsForFolder(const fs::path& folderAbs);

protected:
    void OnGUI() override;

private:
    static wstring ToStr(FsAction fsAction);
    bool IsInterestingFile(const fs::path& p);

    // AssetDB 이벤트 수신(내부에서 dirty 마킹)
    void OnAssetEvent(const AssetEvent& e);


    // 디버그/검증용: 어떤 폴더가 더티인지 확인
    bool IsFolderDirty(const fs::path& folderAbs) const;

    // 폴더를 refresh(캐시 업데이트)하고 dirty를 해제
    void RefreshFolder(const fs::path& folderAbs);

    static fs::path ParentFolderOfEvent(const AssetEvent& e);

private:
    void DrawLeftFolderTree();
    void DrawFolderNode(const fs::path& folderAbs);

    void DrawRightUnityStyle();
    void DrawToolbarRow();      // breadcrumb + search + view toggle
    void DrawBreadcrumb();
    void DrawSearchBox();
    void DrawViewToggle();

    void DrawItemsGrid(const std::vector<BrowserItem>& items);
    void DrawItemsList(const std::vector<BrowserItem>& items);

    bool PassSearchFilter(const BrowserItem& it) const;

    static wstring DisplayName(const fs::path& p);


private:
    DirectoryWatcherWin watcher;
    FsEventDebouncer debouncer;
    AssetDatabase assetDatabase;

    ThreadSafeQueue<FsEvent> eventThreadQueue;
    vector<FsEvent> pendingEvents;
    vector<FsEvent> readyEvents;

    unordered_set<wstring> _dirtyFolders;
    fs::path _root;
    fs::path _currentFolder;

    // 선택 상태(파일)
    fs::path _selectedPath;

    ViewMode _viewMode = ViewMode::Grid;

    // 유니티 느낌: 아이콘 크기 조절
    float _thumbSize = 72.0f;     // 아이콘 박스 크기
    float _thumbPad = 10.0f;     // 아이템 간격
    float _leftPaneWidth = 260.0f;

    // 검색
    std::string _search;          // ImGui InputText는 char*가 편함
};

