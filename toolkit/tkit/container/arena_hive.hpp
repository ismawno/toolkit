#pragma once

#include "tkit/container/hive.hpp"
#include "tkit/container/arena_array.hpp"

namespace TKit
{
template <typename T> using ArenaHive = Hive<T, ArenaAllocation<T>>;
}
