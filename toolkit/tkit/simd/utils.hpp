#pragma once

#include "tkit/utils/alias.hpp"
#include <concepts>

namespace TKit::Detail
{
template <typename T>
concept Float = std::same_as<T, f32> || std::same_as<T, f64>;

template <typename T>
concept UnsignedInteger = std::same_as<T, u8> || std::same_as<T, u16> || std::same_as<T, u32> || std::same_as<T, u64>;
template <typename T>
concept SignedInteger = std::same_as<T, i8> || std::same_as<T, i16> || std::same_as<T, i32> || std::same_as<T, i64>;

template <typename T>
concept Integer = UnsignedInteger<T> || SignedInteger<T>;

template <usize Lanes> constexpr usize MaskSize()
{
    if constexpr (Lanes < 8)
        return 8;
    return Lanes;
}

} // namespace TKit::Detail
