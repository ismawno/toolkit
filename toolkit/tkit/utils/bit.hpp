#pragma once

#include <bit>

namespace TKit::Bit
{
template <typename T> constexpr bool IsPowerOfTwo(const T val)
{
    return val > 0 && std::has_single_bit(static_cast<std::make_unsigned_t<T>>(val));
}

template <typename T> constexpr T NextPowerOfTwo(const T val)
{
    return val == 0 ? 1 : static_cast<T>(std::bit_ceil(static_cast<std::make_unsigned_t<T>>(val)));
}

template <typename T> constexpr T PrevPowerOfTwo(const T val)
{
    return val == 0 ? 0 : static_cast<T>(std::bit_floor(static_cast<std::make_unsigned_t<T>>(val)));
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
    return val == static_cast<T>(-1);
}
} // namespace TKit::Bit
