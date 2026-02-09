#pragma once
#include "Guid.h"
#include "Handle.h"

struct GuidRef
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

    uint64 GetInstanceId() const noexcept
    {
        return guid.GetInstanceId();
    }

    uint64 GetLocalId() const noexcept
    {
        return guid.GetLocalId();
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

struct GuidRefHash
{
    std::size_t operator()(const GuidRef& r) const noexcept
    {
        return GuidHash{}(r.guid);
    }
};

using GuidRefSet = std::unordered_set<GuidRef, GuidRefHash, GuidRefEq>;