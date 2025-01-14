#pragma once

#include "tkit/core/api.hpp"
#include "tkit/core/alias.hpp"
#include "tkit/core/concepts.hpp"
#include <functional>

namespace TKit
{
// This is a tuple of hashable elements, useful when you need to hash multiple elements at the same time. Originally, I
// had implemented a commutative version, which had considerably more overhead and had to use some obscure compile time
// for loop to work (see cpp-kit). At the end of the day tho, I always took extra care to avoid that overhead by making
// sure I was good to go with the default non-commutative one, so I wont be including a commutative tuple for this
// library

/**
 * @brief A tuple of hashable elements.
 *
 * Useful when you need to hash multiple elements at the same time. The elements must be hashable, meaning they must
 * have a std::hash specialization.
 *
 * @tparam H
 */
template <Hashable... H> struct HashableTuple
{
    using Tuple = std::tuple<H...>;

    constexpr HashableTuple() noexcept = default;
    explicit constexpr HashableTuple(const H &...p_Elements) noexcept : Elements(p_Elements...)
    {
    }
    explicit(false) constexpr HashableTuple(const Tuple &p_Elements) noexcept : Elements(p_Elements)
    {
    }

    /**
     * @brief Get the element at index `I`.
     *
     */
    template <usize I> constexpr auto &Get() const noexcept
    {
        return std::get<I>(Elements);
    }

    /**
     * @brief Get the element at index `I`.
     *
     */
    template <usize I> constexpr auto &Get() noexcept
    {
        return std::get<I>(Elements);
    }

    /**
     * @brief Get the element of type `T` if `T` is unique in the tuple. If `T` is not unique, a compile time error will
     * be raised.
     *
     */
    template <Hashable T> constexpr const T &Get() const noexcept
    {
        return std::get<T>(Elements);
    }

    /**
     * @brief Get the element of type `T` if `T` is unique in the tuple. If `T` is not unique, a compile time error will
     * be raised.
     *
     */
    template <Hashable T> constexpr T &Get() noexcept
    {
        return std::get<T>(Elements);
    }

    constexpr HashableTuple &operator=(const Tuple &p_Elements) noexcept
    {
        Elements = p_Elements;
        return *this;
    }

    /**
     * @brief Compute the hash of the tuple.
     *
     */
    constexpr usize operator()() const noexcept
    {
        usize seed = 0x517cc1b7;
        std::apply([&seed](const auto &...elements) { (hashSeed(seed, elements), ...); }, Elements);
        return seed;
    }

    constexpr bool operator==(const HashableTuple &p_Other) const noexcept
    {
        return Elements == p_Other.Elements;
    }

    Tuple Elements;

  private:
    template <Hashable T> static constexpr void hashSeed(usize &p_Seed, const T &p_Hashable) noexcept
    {
        const std::hash<T> hasher;
        p_Seed ^= hasher(p_Hashable) + 0x9e3779b9 + (p_Seed << 6) + (p_Seed >> 2);
    }
};
} // namespace TKit