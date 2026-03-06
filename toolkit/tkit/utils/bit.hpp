#pragma once

#include "tkit/utils/alias.hpp"
#include <bit>

namespace TKit
{
template <typename T> constexpr bool IsPowerOfTwo(const T val)
{
    return val > 0 && std::has_single_bit(scast<std::make_unsigned_t<T>>(val));
}

template <typename T> constexpr T NextPowerOfTwo(const T val)
{
    return val == 0 ? 1 : T(std::bit_ceil(scast<std::make_unsigned_t<T>>(val)));
}

template <typename T> constexpr T PrevPowerOfTwo(const T val)
{
    return val == 0 ? 0 : T(std::bit_floor(scast<std::make_unsigned_t<T>>(val)));
}
template <typename T> constexpr T NoneOf(const T val)
{
    return val == 0;
}
template <typename T> constexpr T AnyOf(const T val)
{
    return val != 0;
}
template <typename T> constexpr T AllOf(const T val)
{
    return val == T(-1);
}
} // namespace TKit
