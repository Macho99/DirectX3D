#pragma once
struct Handle
{
    uint32 index = UINT32_MAX;
    uint32 gen = 0;

    bool IsValid() const { return index != UINT32_MAX; }

    bool operator==(const Handle& rhs) const
    {
        return index == rhs.index && gen == rhs.gen;
    }
};