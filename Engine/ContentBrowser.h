#pragma once
#include "EditorWindow.h"
#include "FolderTreeCache.h"

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

    void GetCurMetaFiles();

protected:
    void OnGUI() override;

private:
    void DrawLeftFolderTree();
    void DrawFolderNodeRecursive(FolderTreeCache::Node* node);

    void DrawRightUnityStyle();
    void DrawToolbarRow();
    void DrawBreadcrumb();
    void DrawSearchBox();
    void DrawViewToggle();

    void DrawItemsGrid();
    void DrawItemsList();

    static wstring DisplayName(const fs::path& p);


private:
    void SetCurrentFolder(const fs::path& folderAbs);
    bool SafeEquivalent(const fs::path& a, const fs::path& b);

    //bool needRefresh = true;

    fs::path _root;
    fs::path _currentFolder;

    AssetId _selectedId;

    ViewMode _viewMode = ViewMode::Grid;

    // 유니티 느낌: 아이콘 크기 조절
    float _thumbSize = 72.0f;     // 아이콘 박스 크기
    float _thumbPad = 10.0f;     // 아이템 간격
    float _leftPaneWidth = 260.0f;

    // 검색
    std::string _search;          // ImGui InputText는 char*가 편함
    vector<MetaFile*> _curMetaFiles;
    FolderTreeCache _tree;
};

