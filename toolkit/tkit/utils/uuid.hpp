#pragma once

#include "tkit/utils/concepts.hpp"

namespace TKit
{
template <Hashable ID = u64> struct UUID
{
    UUID() = default;
    UUID(const ID p_ID) : Value(p_ID)
    {
    }

    static UUID Random()
    {
    }
    operator ID() const
    {
        return Value;
    }

    std::strong_ordering operator<=>(const UUID &p_Other) const = default;

    ID Value;
};
} // namespace TKit

template <TKit::Hashable ID> struct std::hash<TKit::UUID<ID>>
{
    std::size_t operator()(const TKit::UUID<ID> &p_UUID) const
    {
        return std::hash<ID>()(p_UUID.Value);
    }
};