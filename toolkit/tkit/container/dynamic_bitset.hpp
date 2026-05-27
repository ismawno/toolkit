#pragma once

#include "tkit/container/bitset.hpp"
#include "tkit/container/dynamic_array.hpp"

namespace TKit
{
using DynamicBitSet = BitSet<DynamicAllocation<u64>>;
}
