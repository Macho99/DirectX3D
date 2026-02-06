#pragma once
#include "Guid.h"
#include "Handle.h"

class GuidRef
{
public:
    GuidRef() : guid(), cached() {}
    GuidRef(Guid guid) : guid(guid), cached() {}
    GuidRef(const GuidRef& other) : guid(other.guid), cached(other.cached) {}
    GuidRef(Guid guid, Handle handle) : guid(guid), cached(handle) {}

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(guid));
    }

    bool operator==(const GuidRef& rhs) const noexcept
    {
        return guid == rhs.guid;
    }

    bool operator!=(const GuidRef& rhs) const noexcept
    {
        return !(*this == rhs);
    }

    bool IsValid() const noexcept
    {
        return guid.IsValid();
    }

public:
    Guid guid;
    mutable Handle cached;
};

struct GuidRefEq
{
    bool operator()(const GuidRef& a, const GuidRef& b) const noexcept
    {
        return a.guid == b.guid;
    }
};

// 2) 해시 : guid만 해시 (cached 제외)
// 이미 GuidHash가 있다면 그걸 그대로 재사용
struct GuidRefHash
{
    std::size_t operator()(const GuidRef& r) const noexcept
    {
        return GuidHash{}(r.guid);
    }
};

using GuidRefSet = std::unordered_set<GuidRef, GuidRefHash, GuidRefEq>;