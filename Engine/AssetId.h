#pragma once
#include "cereal/cereal.hpp"

struct AssetId
{
public:
    AssetId() : leftId(0), rightId(0) {}
    AssetId(uint64 instanceId, uint64 localId) : leftId(instanceId), rightId(localId) {}

    bool IsValid() const { return leftId != 0 && rightId != 0; }

    bool operator==(const AssetId& rhs) const noexcept
    {
        return leftId == rhs.leftId && rightId == rhs.rightId;
    }
    bool operator!=(const AssetId& rhs) const noexcept { return !(*this == rhs); }

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(leftId));
        ar(CEREAL_NVP(rightId));
    }

    static AssetId CreateAssetId();

    uint64 GetLeftId() const { return leftId; }
    uint64 GetRightId() const { return rightId; }
    wstring ToWString() const
    {
        return std::to_wstring(leftId) + L"_" + std::to_wstring(rightId);
    }
    string ToString() const
    {
        return std::to_string(leftId) + "_" + std::to_string(rightId);
    }

private:
    uint64 leftId;
    uint64 rightId;

    friend struct AssetIdHash;
};

struct AssetIdHash
{
    std::size_t operator()(const AssetId& g) const noexcept
    {
        std::size_t h1 = std::hash<uint64>{}(g.leftId);
        std::size_t h2 = std::hash<uint64>{}(g.rightId);
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    }
};