#pragma once

#include "tkit/container/hive.hpp"
#include "tkit/container/static_array.hpp"

namespace TKit
{
template <typename T, usize Capacity> using StaticHive = Hive<T, StaticAllocation<T, Capacity>>;
template <typename T> using StaticHive4 = StaticHive<T, 4>;
template <typename T> using StaticHive8 = StaticHive<T, 8>;
template <typename T> using StaticHive16 = StaticHive<T, 16>;
template <typename T> using StaticHive32 = StaticHive<T, 32>;
template <typename T> using StaticHive64 = StaticHive<T, 64>;
template <typename T> using StaticHive128 = StaticHive<T, 128>;
template <typename T> using StaticHive196 = StaticHive<T, 196>;
template <typename T> using StaticHive256 = StaticHive<T, 256>;
template <typename T> using StaticHive384 = StaticHive<T, 384>;
template <typename T> using StaticHive512 = StaticHive<T, 512>;
template <typename T> using StaticHive768 = StaticHive<T, 768>;
template <typename T> using StaticHive1024 = StaticHive<T, 1024>;
} // namespace TKit
