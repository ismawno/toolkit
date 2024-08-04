#pragma once

#include "kit/core/core.hpp"
#include "kit/core/alias.hpp"
#include <functional>

KIT_NAMESPACE_BEGIN

template <typename T>
concept Hashable = requires(T a) {
    {
        std::hash<T>()(a)
    } -> std::convertible_to<usz>;
};

template <Hashable... H> struct HashableTuple
{
    HashableTuple() = default;
    explicit HashableTuple(const H &...p_Elements) : Elements(p_Elements...)
    {
    }
    explicit(false) HashableTuple(const Tuple<H...> &p_Elements) : Elements(p_Elements)
    {
    }

    template <usz I> auto &Get() const
    {
        return std::get<I>(Elements);
    }
    template <usz I> auto &Get()
    {
        return std::get<I>(Elements);
    }

    template <Hashable T> const T &Get() const
    {
        return std::get<T>(Elements);
    }
    template <Hashable T> T &Get()
    {
        return std::get<T>(Elements);
    }

    HashableTuple &operator=(const Tuple<H...> &p_Elements)
    {
        Elements = p_Elements;
        return *this;
    }

    usz operator()() const
    {
        usz seed = 0x517cc1b7;
        std::apply([&seed](const auto &...elements) { (hashSeed(seed, elements), ...); }, Elements);
        return seed;
    }

    bool operator==(const HashableTuple &p_Other) const
    {
        return Elements == p_Other.Elements;
    }

    Tuple<H...> Elements;

  private:
    template <Hashable T> static void hashSeed(usz &p_Seed, const T &p_Hashable)
    {
        const std::hash<T> hasher;
        p_Seed ^= hasher(p_Hashable) + 0x9e3779b9 + (p_Seed << 6) + (p_Seed >> 2);
    }
};

KIT_NAMESPACE_END