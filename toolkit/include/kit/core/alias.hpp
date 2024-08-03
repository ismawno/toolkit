#pragma once

#include "kit/core/core.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <tuple>

KIT_NAMESPACE_BEGIN

// These two are just consistency aliases
using float32_t = float;
using float64_t = double;

// These are aliases to get rid of the std:: prefix
using size_t = std::size_t;

using uint8_t = std::uint8_t;
using uint16_t = std::uint16_t;
using uint32_t = std::uint32_t;
using uint64_t = std::uint64_t;

using int8_t = std::int8_t;
using int16_t = std::int16_t;
using int32_t = std::int32_t;
using int64_t = std::int64_t;

// Fits better with the naming convention
using String = std::string;
using Byte = std::byte;

// And finally these are nice to have in case we want to change the container/allocator type easily
template <typename T> using DynamicArray = std::vector<T>;

template <typename Key, typename Value, typename Hash = std::hash<Key>, typename OpEqual = std::equal_to<Key>>
using HashMap = std::unordered_map<Key, Value, Hash, OpEqual>;

template <typename Key, typename Value, typename Hash = std::hash<Key>, typename OpEqual = std::equal_to<Key>>
using HashSet = std::unordered_set<Key, Value, Hash, OpEqual>;

template <typename... Args> using Tuple = std::tuple<Args...>;

KIT_NAMESPACE_END