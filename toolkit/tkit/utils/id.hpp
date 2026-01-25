#pragma once

#include "tkit/utils/hash.hpp"

namespace TKit
{
template <Hashable ID = u64> struct Id
{
    Id() = default;
    Id(const ID id) : Value(id)
    {
    }

    static Id Random()
    {
    }
    operator ID() const
    {
        return Value;
    }

    std::strong_ordering operator<=>(const Id &other) const = default;

    ID Value;
};
} // namespace TKit

template <TKit::Hashable ID> struct std::hash<TKit::Id<ID>>
{
    std::size_t operator()(const TKit::Id<ID> &uuid) const
    {
        return std::hash<ID>()(uuid.Value);
    }
};
