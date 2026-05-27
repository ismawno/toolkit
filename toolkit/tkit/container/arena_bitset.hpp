#pragma once

#include "tkit/container/bitset.hpp"
#include "tkit/container/arena_array.hpp"

namespace TKit
{
using ArenaBitSet = BitSet<ArenaAllocation<u64>>;
}
