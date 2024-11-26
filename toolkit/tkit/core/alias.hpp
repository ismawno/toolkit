#pragma once

#include "tkit/core/api.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <string_view>
#include <string>

namespace TKit
{
// Add a namespace so that other libraries can adopt them...
namespace Alias
{

// About aliases:
// Regarding primitive types, I have always liked short and informative names such as u32, f32, etc. I havent seen
// this convention in many c++ projects around, so Im not sure if its the best option, specially for the size_t
// case, which doesnt have an immediate alias I can think of.

// Regarding container types, its nice to have some of them aliases in case you want to quickly change the container
// type or allocator. This is not the case for std::string or std::byte, which dont have a meaningful justification
// for an alias, but I have added them for "increased" consistency.

using f32 = float;
using f64 = double;

using usize = std::size_t;
using uptr = std::uintptr_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

// This is a special hash overload for associative containers that uses transparent hashing and comparison (advised by
// sonarlint)
struct StringHash
{
    using is_transparent = void; // Enables heterogeneous operations.

    usize operator()(std::string_view sv) const
    {
        std::hash<std::string_view> hasher;
        return hasher(sv);
    }
};

template <typename T> struct HashAlias
{
    using Type = std::hash<T>;
};
template <> struct HashAlias<std::string>
{
    using Type = StringHash;
};

template <typename T> struct OpAlias
{
    using Type = std::equal_to<T>;
};
template <> struct OpAlias<std::string>
{
    using Type = std::equal_to<>;
};

// These are nice to have in case I want to change the container/allocator type easily or to use the transparent
// operations
template <typename T> using DynamicArray = std::vector<T>;
template <typename T> using Deque = std::deque<T>;

template <typename Key, typename Value, typename Hash = typename HashAlias<Key>::Type,
          typename OpEqual = typename OpAlias<Key>::Type>
using HashMap = std::unordered_map<Key, Value, Hash, OpEqual>;

template <typename Value, typename Hash = typename HashAlias<Value>::Type,
          typename OpEqual = typename OpAlias<Value>::Type>
using HashSet = std::unordered_set<Value, Hash, OpEqual>;
} // namespace Alias

//... and use them immediately in the toolkit namespace
using namespace Alias;
} // namespace TKit