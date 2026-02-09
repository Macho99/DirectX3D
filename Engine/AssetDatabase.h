#pragma once

#include "MetaStore.h"
#include "DirectoryWatcherWin.h"

struct AssetEvent
{
    FsEvent fs;
    Guid guid;
};

class AssetDatabase
{
public:
    AssetDatabase() {}
    ~AssetDatabase() {}

    using Listener = function<void(const AssetEvent&)>;

    void AddListener(Listener l);

    // 메인 스레드에서 호출(디바운스 끝난 ready 이벤트)
    void OnFileEvent(const FsEvent& e);

    bool TryGetGuidByPath(const fs::path& absPath, Guid& out) const;
    bool TryGetPathByGuid(const Guid& guid, fs::path& out) const;

    // ContentBrowser가 폴더 갱신 시 호출할 수 있게: "현재 폴더의 직계 자식 파일들"
    vector<fs::path> ListChildrenFiles(const fs::path& folderAbs) const;

private:
    void Emit(const AssetEvent& e);
    void Upsert(const fs::path& absPath, const FsEvent& srcEvent);
    void Remove(const fs::path& absPath, const FsEvent& srcEvent);

private:
    mutable mutex _mtx;
    vector<Listener> _listeners;

    // 경로는 key로 wstring이 편합니다(대소문자/정규화는 다음 단계에서)
    unordered_map<wstring, Guid> _pathToGuid;
    unordered_map<Guid, fs::path, GuidHash> _guidToPath;
};
