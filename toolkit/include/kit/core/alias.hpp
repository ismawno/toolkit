#pragma once

#include "kit/core/core.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>

KIT_NAMESPACE_BEGIN

// About aliases:
// Regarding primitive types, I have always liked short and informative names such as u32, f32, etc. I havent seen this
// convention in many c++ projects around, so Im not sure if its the best option, specially for the size_t case, which
// doesnt have an immediate alias I can think of.

// Regarding container types, its nice to have some of them aliases in case you want to quickly change the container
// type or allocator. This is not the case for std::string or std::byte, which dont have a meaningful justification for
// an alias, but I have added them for "increased" consistency.

using f32 = float;
using f64 = double;

using usz = std::size_t;
using uptr = std::uintptr_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

// These are nice to have in case we want to change the container/allocator type easily
template <typename T> using DynamicArray = std::vector<T>;

template <typename T> using Deque = std::deque<T>;

template <typename Key, typename Value, typename Hash = std::hash<Key>, typename OpEqual = std::equal_to<Key>>
using HashMap = std::unordered_map<Key, Value, Hash, OpEqual>;

template <typename Value, typename Hash = std::hash<Value>, typename OpEqual = std::equal_to<Value>>
using HashSet = std::unordered_set<Value, Hash, OpEqual>;

KIT_NAMESPACE_END