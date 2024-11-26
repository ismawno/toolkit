#pragma once

#include "tkit/core/concepts.hpp"

namespace TKit
{
template <Hashable ID = u64> struct UUID
{
    UUID() = default;
    UUID(const ID p_ID) noexcept : Value(p_ID)
    {
    }

    static UUID Random() noexcept
    {
    }
    operator ID() const noexcept
    {
        return Value;
    }

    std::strong_ordering operator<=>(const UUID &p_Other) const noexcept = default;

    ID Value;
};
} // namespace TKit

template <TKit::Hashable ID> struct std::hash<TKit::UUID<ID>>
{
    TKIT_API usize operator()(const TKit::UUID<ID> &p_UUID) const noexcept
    {
        return std::hash<ID>()(p_UUID.Value);
    }
};