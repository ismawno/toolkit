#pragma once

#include "tkit/container/hive.hpp"
#include "tkit/container/tier_array.hpp"

namespace TKit
{
template <typename T> using TierHive = Hive<T, TierAllocation<T>>;
}
