#pragma once

#include "tkit/container/fixed_array.hpp"
#include "tkit/container/array.hpp"

namespace TKit
{
template <typename T, usize Capacity> struct StaticAllocation
{
    static constexpr ArrayType Type = Array_Static;
    template <typename U> using Rebind = StaticAllocation<U, Capacity>;

    StaticAllocation() = default;
    struct alignas(T) Element
    {
        std::byte Data[sizeof(T)];
    };

    constexpr usize GetCapacity() const
    {
        return Capacity;
    }

    static_assert(sizeof(Element) == sizeof(T), "Element size is not equal to T size");
    static_assert(alignof(Element) == alignof(T), "Element alignment is not equal to T alignment");

    FixedArray<Element, Capacity> Data{};
    usize Size = 0;
};
template <typename T, usize Capacity> using StaticArray = Array<T, StaticAllocation<T, Capacity>>;
template <usize Capacity> using StaticString = Array<char, StaticAllocation<char, Capacity + 1>>;

template <typename T> using StaticArray4 = StaticArray<T, 4>;
template <typename T> using StaticArray8 = StaticArray<T, 8>;
template <typename T> using StaticArray16 = StaticArray<T, 16>;
template <typename T> using StaticArray32 = StaticArray<T, 32>;
template <typename T> using StaticArray64 = StaticArray<T, 64>;
template <typename T> using StaticArray128 = StaticArray<T, 128>;
template <typename T> using StaticArray196 = StaticArray<T, 196>;
template <typename T> using StaticArray256 = StaticArray<T, 256>;
template <typename T> using StaticArray384 = StaticArray<T, 384>;
template <typename T> using StaticArray512 = StaticArray<T, 512>;
template <typename T> using StaticArray768 = StaticArray<T, 768>;
template <typename T> using StaticArray1024 = StaticArray<T, 1024>;

using StaticString4 = StaticString<4>;
using StaticString8 = StaticString<8>;
using StaticString16 = StaticString<16>;
using StaticString32 = StaticString<32>;
using StaticString64 = StaticString<64>;
using StaticString128 = StaticString<128>;
using StaticString196 = StaticString<196>;
using StaticString256 = StaticString<256>;
using StaticString384 = StaticString<384>;
using StaticString512 = StaticString<512>;
using StaticString768 = StaticString<768>;
using StaticString1024 = StaticString<1024>;
} // namespace TKit
