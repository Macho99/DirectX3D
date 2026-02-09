#pragma once
#include "cereal/cereal.hpp"

struct Guid
{
public:
    Guid() : instanceId(0), localId(0) {}
    Guid(uint64 instanceId, uint64 localId) : instanceId(instanceId), localId(localId) {}

    bool IsValid() const { return localId != 0; }

    bool operator==(const Guid& rhs) const noexcept
    {
        return instanceId == rhs.instanceId && localId == rhs.localId;
    }
    bool operator!=(const Guid& rhs) const noexcept { return !(*this == rhs); }

    template<class Archive>
    void load(Archive& ar)
    {
        instanceId = s_currentInstanceId;
        ar(CEREAL_NVP(localId));
    }

    template<class Archive>
    void save(Archive& ar) const
    {
        ar(CEREAL_NVP(localId));
    }

    static Guid CreateNewObjectGuid();
    static Guid CreateNewAssetGuid();

    static void SetCurrentInstanceId(uint64 id);

    uint64 GetInstanceId() const { return instanceId; }
    uint64 GetLocalId() const { return localId; }

private:
    static uint64 s_currentInstanceId;

    uint64 instanceId; // 런타임 인스턴스(씬/프리팹 인스턴스) 단위
    uint64 localId;    // 에디터/빌드에서 고정되는 로컬 ID (저장 대상)

    friend struct GuidHash;
};

struct GuidHash
{
    std::size_t operator()(const Guid& g) const noexcept
    {
        std::size_t h1 = std::hash<uint64>{}(g.instanceId);
        std::size_t h2 = std::hash<uint64>{}(g.localId);
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    }
};