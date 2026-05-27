#pragma once

#include "tkit/container/bitset.hpp"
#include "tkit/container/static_array.hpp"

namespace TKit
{

template <usize Capacity> using StaticBitSet = BitSet<StaticAllocation<u64, 1 + Capacity / 64>>;
using StaticBitSet256 = StaticBitSet<256>;
using StaticBitSet512 = StaticBitSet<512>;
using StaticBitSet1024 = StaticBitSet<1024>;
using StaticBitSet2048 = StaticBitSet<2048>;
using StaticBitSet4096 = StaticBitSet<4096>;
using StaticBitSet8192 = StaticBitSet<8192>;
using StaticBitSet16384 = StaticBitSet<16384>;
using StaticBitSet32768 = StaticBitSet<32768>;
using StaticBitSet65536 = StaticBitSet<65536>;

} // namespace TKit
