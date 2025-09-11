#pragma once

#include <bit>

namespace TKit
{
template <typename T> constexpr bool IsPowerOfTwo(const T p_Val)
{
    return p_Val > 0 && std::has_single_bit(static_cast<std::make_unsigned_t<T>>(p_Val));
}

template <typename T> constexpr T NextPowerOfTwo(const T p_Val)
{
    return p_Val == 0 ? 1 : static_cast<T>(std::bit_ceil(static_cast<std::make_unsigned_t<T>>(p_Val)));
}

template <typename T> constexpr T PrevPowerOfTwo(const T p_Val)
{
    return p_Val == 0 ? 0 : static_cast<T>(std::bit_floor(static_cast<std::make_unsigned_t<T>>(p_Val)));
}
} // namespace TKit
