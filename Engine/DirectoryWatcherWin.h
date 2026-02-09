#pragma once

enum class FsAction
{
    Added,
    Removed,
    Modified,
    Renamed
};

struct FsEvent
{
    FsAction action{};
    fs::path absPath;
    fs::path oldAbsPath;
};

class DirectoryWatcherWin
{
public:
    using Callback = std::function<void(const FsEvent&)>;

    DirectoryWatcherWin() = default;
    ~DirectoryWatcherWin();

    DirectoryWatcherWin(const DirectoryWatcherWin&) = delete;
    DirectoryWatcherWin& operator=(const DirectoryWatcherWin&) = delete;

    bool Start(const fs::path& folderAbs, bool recursive, Callback cb);
    void Stop();

private:
    void ThreadMain();
    void HandleBuffer(const uint8_t* data, DWORD bytes);
    fs::path MakeAbsPathFromRelativeName(const std::wstring& relativeName) const;

private:
    fs::path _folderAbs;
    bool _recursive = true;
    Callback _cb;

    HANDLE _dir = INVALID_HANDLE_VALUE;
    std::thread _thread;
    std::atomic<bool> _running{ false };

    fs::path _renameOldAbs;
};
