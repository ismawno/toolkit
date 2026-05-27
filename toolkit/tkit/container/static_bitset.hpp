#pragma once

#include "tkit/container/bitset.hpp"
#include "tkit/container/static_array.hpp"

namespace TKit
{
template <usize Capacity> using StaticBitSet = BitSet<StaticAllocation<u64, Capacity>>;
template <typename T> using StaticBitSet4 = StaticBitSet<4>;
template <typename T> using StaticBitSet8 = StaticBitSet<8>;
template <typename T> using StaticBitSet16 = StaticBitSet<16>;
template <typename T> using StaticBitSet32 = StaticBitSet<32>;
template <typename T> using StaticBitSet64 = StaticBitSet<64>;
template <typename T> using StaticBitSet128 = StaticBitSet<128>;
template <typename T> using StaticBitSet196 = StaticBitSet<196>;
template <typename T> using StaticBitSet256 = StaticBitSet<256>;
template <typename T> using StaticBitSet384 = StaticBitSet<384>;
template <typename T> using StaticBitSet512 = StaticBitSet<512>;
template <typename T> using StaticBitSet768 = StaticBitSet<768>;
template <typename T> using StaticBitSet1024 = StaticBitSet<1024>;
} // namespace TKit
