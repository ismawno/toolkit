#pragma once

#include "tkit/container/hive.hpp"
#include "tkit/container/dynamic_array.hpp"

namespace TKit
{
template <typename T> using DynamicHive = Hive<T, DynamicAllocation<T>>;
}
