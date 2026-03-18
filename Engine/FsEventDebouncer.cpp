#include "pch.h"
#include "FsEventDebouncer.h"

void FsEventDebouncer::Push(const FsEvent& e)
{
    // Rename은 페어가 꼬일 수 있어서, 여기선 즉시 통과시키는 쪽이 안전합니다.
    if (e.action == FsAction::Renamed)
    {
        _immediate.push_back(e);
        return;
    }

    auto now = Clock::now();
    auto key = e.absPath.wstring();

    auto& ent = _map[key];
    ent.event = Merge(ent.event, e);
    ent.lastUpdate = now;
    ent.hasValue = true;
}

void FsEventDebouncer::PopReady(uint32_t debounceMs, std::vector<FsEvent>& out)
{
    // 1) 즉시 통과(rename 등)
    if (!_immediate.empty())
    {
        out.insert(out.end(), _immediate.begin(), _immediate.end());
        _immediate.clear();
    }

    auto now = Clock::now();

    for (auto it = _map.begin(); it != _map.end(); )
    {
        const auto& ent = it->second;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - ent.lastUpdate).count();

        if (ent.hasValue && elapsed >= (long long)debounceMs)
        {
            out.push_back(ent.event);
            it = _map.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

FsEvent FsEventDebouncer::Merge(const FsEvent& prevEvent, const FsEvent& newEvent)
{
    if (prevEvent.absPath.empty()) return newEvent;
    if (newEvent.absPath.empty()) return prevEvent;

    // 같은 경로라는 가정
    FsEvent r = prevEvent;

    switch (prevEvent.action)
    {
    case FsAction::Added:
        switch (newEvent.action)
        {
        case FsAction::Added:
        case FsAction::Modified:
            r.action = FsAction::Added;
            break;
        case FsAction::Removed:
            r.action = FsAction::Removed;
            break;
        default:
            r.action = newEvent.action;
            break;
        }
        break;

    case FsAction::Removed:
        switch (newEvent.action)
        {
        case FsAction::Added:
            r.action = FsAction::Added;
            break;
        case FsAction::Removed:
        case FsAction::Modified:
            r.action = FsAction::Removed;
            break;
        default:
            r.action = newEvent.action;
            break;
        }
        break;

    case FsAction::Modified:
        switch (newEvent.action)
        {
        case FsAction::Added:
            r.action = FsAction::Added;
            break;
        case FsAction::Removed:
            r.action = FsAction::Removed;
            break;
        case FsAction::Modified:
            r.action = FsAction::Modified;
            break;
        default:
            r.action = newEvent.action;
            break;
        }
        break;

    default:
            r.action = newEvent.action;
            break;
    }

    // 경로는 동일
    r.absPath = prevEvent.absPath;
    r.oldAbsPath = fs::path(); // rename은 여기서 안 처리
    return r;
}