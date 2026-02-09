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

FsEvent FsEventDebouncer::Merge(const FsEvent& a, const FsEvent& b)
{
    if (a.absPath.empty()) return b;
    if (b.absPath.empty()) return a;

    // 같은 경로라는 가정
    FsEvent r = a;

    auto pri = [](FsAction act) -> int
        {
            // 우선순위: Removed > Added > Modified
            switch (act)
            {
            case FsAction::Removed:  return 3;
            case FsAction::Added:    return 2;
            case FsAction::Modified: return 1;
            default: return 0;
            }
        };

    // 우선순위 높은 쪽으로
    r.action = (pri(b.action) > pri(a.action)) ? b.action : a.action;

    // 경로는 동일
    r.absPath = a.absPath;
    r.oldAbsPath = fs::path(); // rename은 여기서 안 처리
    return r;
}