#pragma once

#include "cereal/cereal.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"

struct BatchInfo
{
    static constexpr const char* Extension = ".batchInfo";

    vector<string> ModelNames;
    vector<string> MaterialNames;

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(
            cereal::make_nvp("ModelNames", ModelNames),
            cereal::make_nvp("MaterialNames", MaterialNames)
        );
    }
};
