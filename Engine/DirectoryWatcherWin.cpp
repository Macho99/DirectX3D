#include "pch.h"
#include "DirectoryWatcherWin.h"

#include <vector>

DirectoryWatcherWin::~DirectoryWatcherWin()
{
    Stop();
}

bool DirectoryWatcherWin::Start(const fs::path& folderAbs, bool recursive, Callback cb)
{
    Stop();

    _folderAbs = folderAbs;
    _recursive = recursive;
    _cb = std::move(cb);
    _renameOldAbs.clear();

    _dir = ::CreateFileW(
        _folderAbs.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    if (_dir == INVALID_HANDLE_VALUE)
        return false;

    _running = true;
    _thread = std::thread([this]() { ThreadMain(); });
    return true;
}

void DirectoryWatcherWin::Stop()
{
    _running = false;

    if (_dir != INVALID_HANDLE_VALUE)
        ::CancelIoEx(_dir, nullptr);

    if (_thread.joinable())
        _thread.join();

    if (_dir != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(_dir);
        _dir = INVALID_HANDLE_VALUE;
    }

    _renameOldAbs.clear();
}

void DirectoryWatcherWin::ThreadMain()
{
    std::vector<uint8_t> buffer(64 * 1024);
    DWORD bytesReturned = 0;

    while (_running)
    {
        BOOL ok = ::ReadDirectoryChangesW(
            _dir,
            buffer.data(),
            (DWORD)buffer.size(),
            _recursive ? TRUE : FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_LAST_WRITE |
            FILE_NOTIFY_CHANGE_SIZE,
            &bytesReturned,
            nullptr,
            nullptr);

        if (!ok || bytesReturned == 0)
        {
            if (!_running) break;
            ::Sleep(10);
            continue;
        }

        HandleBuffer(buffer.data(), bytesReturned);
    }
}

fs::path DirectoryWatcherWin::MakeAbsPathFromRelativeName(const std::wstring& relativeName) const
{
    return _folderAbs / fs::path(relativeName);
}

void DirectoryWatcherWin::HandleBuffer(const uint8_t* data, DWORD bytes)
{
    (void)bytes;

    const FILE_NOTIFY_INFORMATION* fni = (const FILE_NOTIFY_INFORMATION*)data;

    while (true)
    {
        std::wstring relName(fni->FileName, fni->FileNameLength / sizeof(WCHAR));
        fs::path abs = MakeAbsPathFromRelativeName(relName);

        if (_cb)
        {
            FsEvent ev{};

            switch (fni->Action)
            {
            case FILE_ACTION_ADDED:
                ev.action = FsAction::Added;
                ev.absPath = abs;
                _cb(ev);
                break;

            case FILE_ACTION_REMOVED:
                ev.action = FsAction::Removed;
                ev.absPath = abs;
                _cb(ev);
                break;

            case FILE_ACTION_MODIFIED:
                ev.action = FsAction::Modified;
                ev.absPath = abs;
                _cb(ev);
                break;

            case FILE_ACTION_RENAMED_OLD_NAME:
                _renameOldAbs = abs;
                break;

            case FILE_ACTION_RENAMED_NEW_NAME:
                ev.action = FsAction::Renamed;
                ev.absPath = abs;
                ev.oldAbsPath = _renameOldAbs;
                _cb(ev);
                _renameOldAbs.clear();
                break;
            }
        }

        if (fni->NextEntryOffset == 0) break;
        fni = (const FILE_NOTIFY_INFORMATION*)((const uint8_t*)fni + fni->NextEntryOffset);
    }
}
