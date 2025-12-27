#pragma once

#include "tkit/utils/alias.hpp"
#include <concepts>
#include <functional>

#ifndef TKIT_HASH_SEED
#    define TKIT_HASH_SEED 0x517cc1b7
#endif

namespace TKit
{
template <typename T>
concept Hashable = requires(T a) {
    { std::hash<std::remove_cvref_t<T>>()(a) } -> std::convertible_to<usize>;
};

} // namespace TKit

namespace TKit::Detail
{
template <Hashable H> constexpr void CombineHashes(size_t &p_Seed, H &&p_Hashable)
{
    const std::hash<std::remove_cvref_t<H>> hasher;
    p_Seed ^= hasher(std::forward<H>(p_Hashable)) + 0x9e3779b9 + (p_Seed << 6) + (p_Seed >> 2);
}
} // namespace TKit::Detail

namespace TKit
{
template <Hashable H> constexpr size_t Hash(H &&p_Hashable)
{
    return std::hash<std::remove_cvref_t<H>>()(std::forward<H>(p_Hashable));
}

template <Hashable... H> constexpr size_t Hash(H &&...p_Hashables)
{
    size_t seed = TKIT_HASH_SEED;
    (Detail::CombineHashes(seed, std::forward<H>(p_Hashables)), ...);
    return seed;
}

template <typename It> constexpr size_t HashRange(const It p_Begin, const It p_End)
{
    size_t seed = TKIT_HASH_SEED;
    for (It it = p_Begin; it != p_End; ++it)
        Detail::CombineHashes(seed, *it);
    return seed;
}

template <Hashable... H> constexpr void HashCombine(size_t &p_Seed, H &&...p_Hashables)
{
    (Detail::CombineHashes(p_Seed, std::forward<H>(p_Hashables)), ...);
}

} // namespace TKit
