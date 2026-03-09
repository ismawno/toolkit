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
template <Hashable H> constexpr void CombineHashes(usz &seed, H &&hashable)
{
    const std::hash<std::remove_cvref_t<H>> hasher;
    seed ^= hasher(std::forward<H>(hashable)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
} // namespace TKit::Detail

namespace TKit
{
template <Hashable H> constexpr usz Hash(H &&hashable)
{
    return std::hash<std::remove_cvref_t<H>>()(std::forward<H>(hashable));
}

template <Hashable... H> constexpr usz Hash(H &&...hashables)
{
    usz seed = TKIT_HASH_SEED;
    (Detail::CombineHashes(seed, std::forward<H>(hashables)), ...);
    return seed;
}

template <typename It> constexpr usz HashRange(const It begin, const It end)
{
    usz seed = TKIT_HASH_SEED;
    for (It it = begin; it != end; ++it)
        Detail::CombineHashes(seed, *it);
    return seed;
}

template <Hashable... H> constexpr void HashCombine(usz &seed, H &&...hashables)
{
    (Detail::CombineHashes(seed, std::forward<H>(hashables)), ...);
}

} // namespace TKit
