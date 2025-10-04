#pragma once

#include "tkit/utils/alias.hpp"

namespace TKit
{
template <typename T>
concept Float = std::same_as<T, f32> || std::same_as<T, f64>;

template <typename T>
concept UnsignedInteger = std::same_as<T, u8> || std::same_as<T, u16> || std::same_as<T, u32> || std::same_as<T, u64>;
template <typename T>
concept SignedInteger = std::same_as<T, i8> || std::same_as<T, i16> || std::same_as<T, i32> || std::same_as<T, i64>;

template <typename T>
concept Integer = UnsignedInteger<T> || SignedInteger<T>;

} // namespace TKit
