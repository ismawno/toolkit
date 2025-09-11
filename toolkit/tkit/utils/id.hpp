#pragma once

#include "tkit/utils/concepts.hpp"

namespace TKit
{
template <Hashable ID = u64> struct Id
{
    Id() = default;
    Id(const ID p_ID) : Value(p_ID)
    {
    }

    static Id Random()
    {
    }
    operator ID() const
    {
        return Value;
    }

    std::strong_ordering operator<=>(const Id &p_Other) const = default;

    ID Value;
};
} // namespace TKit

template <TKit::Hashable ID> struct std::hash<TKit::Id<ID>>
{
    std::size_t operator()(const TKit::Id<ID> &p_UUID) const
    {
        return std::hash<ID>()(p_UUID.Value);
    }
};
