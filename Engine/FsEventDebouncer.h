#pragma once
#include "DirectoryWatcherWin.h"

class FsEventDebouncer
{
public:
    using Clock = std::chrono::steady_clock;

    void Push(const FsEvent& e);

    // debounceMs 지난 이벤트를 out으로 배출
    void PopReady(uint32_t debounceMs, std::vector<FsEvent>& out);

private:
    struct Entry
    {
        FsEvent event{};
        Clock::time_point lastUpdate{};
        bool hasValue = false;
    };

    // 같은 파일에 대한 이벤트 합치기 규칙(최소)
    // - Added + Modified => Added(최초 생성이 더 중요)
    // - Modified 여러 개 => Modified
    // - Removed가 오면 => Removed(최종)
    static FsEvent Merge(const FsEvent& a, const FsEvent& b);

private:
    unordered_map<wstring, Entry> _map;
    vector<FsEvent> _immediate;
};
