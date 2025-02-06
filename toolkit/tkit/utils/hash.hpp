#pragma once

#include "tkit/utils/concepts.hpp"
#include <functional>

#ifndef TKIT_HASH_SEED
#    define TKIT_HASH_SEED 0x517cc1b7
#endif

namespace TKit
{
namespace
{
template <Hashable H> constexpr void hashCombine(size_t &p_Seed, H &&p_Hashable) noexcept
{
    const std::hash<NoCVRef<H>> hasher;
    p_Seed ^= hasher(std::forward<H>(p_Hashable)) + 0x9e3779b9 + (p_Seed << 6) + (p_Seed >> 2);
}
} // namespace

template <Hashable H> constexpr size_t Hash(H &&p_Hashable) noexcept
{
    return std::hash<NoCVRef<H>>()(std::forward<H>(p_Hashable));
}

template <Hashable... H> constexpr size_t Hash(H &&...p_Hashables) noexcept
{
    size_t seed = TKIT_HASH_SEED;
    (hashCombine(seed, std::forward<H>(p_Hashables)), ...);
    return seed;
}

template <typename It> constexpr size_t HashRange(const It p_Begin, const It p_End) noexcept
{
    size_t seed = TKIT_HASH_SEED;
    for (It it = p_Begin; it != p_End; ++it)
        hashCombine(seed, *it);
    return seed;
}

template <Hashable... H> constexpr void HashCombine(size_t &p_Seed, H &&...p_Hashables) noexcept
{
    (hashCombine(p_Seed, std::forward<H>(p_Hashables)), ...);
}

} // namespace TKit