#pragma once
#include "Guid.h"
#include "Handle.h"

template<class T>
class SlotManager
{
public:
    SlotManager();
    ~SlotManager();

    struct Slot
    {
        std::unique_ptr<T> ptr;
        uint32 gen = 1;          // 0은 보통 "없음"으로 쓰기 쉬워서 1부터 시작
    };

    // 생성 + 등록
    //  - guid가 이미 존재하면(중복 로드 등) 교체/에러 정책은 프로젝트에 맞게 결정하세요.
    template<class... Args>
    Handle CreateAndRegister(const Guid& guid, Args&&... args)
    {
        Handle h = AllocateSlot();
        Slot& s = _slots[h.index];

        s.ptr = std::make_unique<T>(std::forward<Args>(args)...);
        // T가 guid를 멤버로 가진다는 가정(예시 타입들은 그렇습니다)
        if constexpr (requires(T t) { t.guid = guid; })
        {
            s.ptr->guid = guid;
        }

        _guidToHandle[guid] = h;
        return h;
    }

    // 외부에서 이미 만들어진 unique_ptr 등록하고 싶을 때
    Handle RegisterExisting(const Guid& guid, std::unique_ptr<T> obj)
    {
        Handle h = AllocateSlot();
        Slot& s = _slots[h.index];

        s.ptr = std::move(obj);
        if (s.ptr)
        {
            if constexpr (requires(T t) { t.guid = guid; })
                s.ptr->guid = guid;
        }

        _guidToHandle[guid] = h;
        return h;
    }

    // Guid로 핸들 얻기
    Handle FindHandle(const Guid& guid) const
    {
        auto it = _guidToHandle.find(guid);
        if (it == _guidToHandle.end())
            return {};
        return it->second;
    }

    // Handle -> 포인터 (세대 검사)
    T* Resolve(const Handle& h)
    {
        if (!h.IsValid()) return nullptr;
        if (h.index >= _slots.size()) return nullptr;

        Slot& s = _slots[h.index];
        if (s.gen != h.gen) return nullptr;
        if (!s.ptr) return nullptr;
        return s.ptr.get();
    }

    const T* Resolve(const Handle& h) const
    {
        return const_cast<SlotManager*>(this)->Resolve(h);
    }

    // Guid -> 포인터 (내부에서 Handle 찾고 Resolve)
    T* Resolve(const Guid& guid)
    {
        return Resolve(FindHandle(guid));
    }

private:

    Handle AllocateSlot()
    {
        if (!_freeIndices.empty())
        {
            uint32 idx = _freeIndices.back();
            _freeIndices.pop_back();
            return Handle{ idx, _slots[idx].gen };
        }

        uint32 idx = static_cast<uint32>(_slots.size());
        _slots.push_back(Slot{});
        return Handle{ idx, _slots[idx].gen };
    }

private:
    vector<Slot> _slots;
    vector<uint32> _freeIndices;
    unordered_map<Guid, Handle, GuidHash> _guidToHandle;
};
