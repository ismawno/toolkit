#pragma once

#include "tkit/memory/memory.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <string_view>
#include <string>

namespace TKit
{
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
template <typename T, typename Allocator = DefaultAllocator<T>> using DynamicArray = std::vector<T, Allocator>;
template <typename T, typename Allocator = DefaultAllocator<T>> using Deque = std::deque<T, Allocator>;

template <typename Key, typename Value, typename Hash = typename HashAlias<Key>::Type,
          typename OpEqual = typename OpAlias<Key>::Type,
          typename Allocator = DefaultAllocator<std::pair<const Key, Value>>>
using HashMap = std::unordered_map<Key, Value, Hash, OpEqual, Allocator>;

template <typename Value, typename Hash = typename HashAlias<Value>::Type,
          typename OpEqual = typename OpAlias<Value>::Type, typename Allocator = DefaultAllocator<Value>>
using HashSet = std::unordered_set<Value, Hash, OpEqual>;
} // namespace TKit
