#pragma once

#include "kit/core/core.hpp"
#include "kit/core/alias.hpp"
#include "kit/core/concepts.hpp"
#include <functional>

KIT_NAMESPACE_BEGIN

// This is a tuple of hashable elements, useful when you need to hash multiple elements at the same time. Originally, I
// had implemented a commutative version, which had considerably more overhead and had to use some obscure compile time
// for loop to work (see cpp-kit). At the end of the day tho, I always took extra care to avoid that overhead by making
// sure I was good to go with the default non-commutative one, so I wont be including a commutative tuple for this
// library

template <Hashable... H> struct KIT_API HashableTuple
{
    using Tuple = std::tuple<H...>;

    HashableTuple() = default;
    explicit HashableTuple(const H &...p_Elements) KIT_NOEXCEPT : Elements(p_Elements...)
    {
    }
    explicit(false) HashableTuple(const Tuple &p_Elements) KIT_NOEXCEPT : Elements(p_Elements)
    {
    }

    template <usz I> auto &Get() KIT_NOEXCEPT const
    {
        return std::get<I>(Elements);
    }
    template <usz I> auto &Get() KIT_NOEXCEPT
    {
        return std::get<I>(Elements);
    }

    template <Hashable T> const T &Get() KIT_NOEXCEPT const
    {
        return std::get<T>(Elements);
    }
    template <Hashable T> T &Get() KIT_NOEXCEPT
    {
        return std::get<T>(Elements);
    }

    HashableTuple &operator=(const Tuple &p_Elements) KIT_NOEXCEPT
    {
        Elements = p_Elements;
        return *this;
    }

    usz operator()() KIT_NOEXCEPT const
    {
        usz seed = 0x517cc1b7;
        std::apply([&seed](const auto &...elements) { (hashSeed(seed, elements), ...); }, Elements);
        return seed;
    }

    bool operator==(const HashableTuple &p_Other) KIT_NOEXCEPT const
    {
        return Elements == p_Other.Elements;
    }

    Tuple Elements;

  private:
    template <Hashable T> static void hashSeed(usz &p_Seed, const T &p_Hashable) KIT_NOEXCEPT
    {
        const std::hash<T> hasher;
        p_Seed ^= hasher(p_Hashable) + 0x9e3779b9 + (p_Seed << 6) + (p_Seed >> 2);
    }
};

KIT_NAMESPACE_END