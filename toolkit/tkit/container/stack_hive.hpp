#pragma once

#include "tkit/container/hive.hpp"
#include "tkit/container/stack_array.hpp"

namespace TKit
{
template <typename T> using StackHive = Hive<T, StackAllocation<T>>;
}
