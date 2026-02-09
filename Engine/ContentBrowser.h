#pragma once
#include "EditorWindow.h"
#include "DirectoryWatcherWin.h"
#include "ThreadSafeQueue.h"
#include "FsEventDebouncer.h"

class ContentBrowser : public EditorWindow
{
    using Super = EditorWindow;
public:
    ContentBrowser();
    ~ContentBrowser();

    void Init(EditorManager* editorManager) override;
    void Update() override;

protected:
    void OnGUI() override;

private:
    static wstring ToStr(FsAction fsAction);
    bool IsInterestingFile(const fs::path& p);
    bool IsMetaFile(const fs::path& p);
    void ProcessMetaFile(FsEvent fsEvent);

private:
    DirectoryWatcherWin watcher;
    FsEventDebouncer debouncer;
    ThreadSafeQueue<FsEvent> eventThreadQueue;
    vector<FsEvent> pendingEvents;
    vector<FsEvent> readyEvents;
};

