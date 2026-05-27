#pragma once

#include "tkit/container/bitset.hpp"
#include "tkit/container/stack_array.hpp"

namespace TKit
{
using StackBitSet = BitSet<StackAllocation<u64>>;
}
