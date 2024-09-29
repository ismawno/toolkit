#pragma once

#include "Kit/core/concepts.hpp"

namespace KIT
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
} // namespace KIT

template <KIT::Hashable ID> struct std::hash<KIT::UUID<ID>>
{
    KIT_API usize operator()(const KIT::UUID<ID> &p_UUID) const noexcept
    {
        return std::hash<ID>()(p_UUID.Value);
    }
};