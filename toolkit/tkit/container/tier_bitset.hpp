#pragma once

#include "tkit/container/bitset.hpp"
#include "tkit/container/tier_array.hpp"

namespace TKit
{
using TierBitSet = BitSet<TierAllocation<u64>>;
}
