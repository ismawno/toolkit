#pragma once

#include "tkit/memory/memory.hpp"
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <string_view>
#include <string>

namespace TKit::Detail
{
struct TKIT_API StringHash
{
    using is_transparent = void; // Enables heterogeneous operations.

    std::size_t operator()(std::string_view sv) const
    {
        std::hash<std::string_view> hasher;
        return hasher(sv);
    }
};

template <typename T> struct HashAlias
{
    using Type = std::hash<T>;
};
template <> struct TKIT_API HashAlias<std::string>
{
    using Type = StringHash;
};

template <typename T> struct OpAlias
{
    using Type = std::equal_to<T>;
};
template <> struct TKIT_API OpAlias<std::string>
{
    using Type = std::equal_to<>;
};
} // namespace TKit::Detail

namespace TKit
{
template <typename Key, typename Value, typename Hash = typename Detail::HashAlias<Key>::Type,
          typename OpEqual = typename Detail::OpAlias<Key>::Type,
          typename Allocator = Memory::STLAllocator<std::pair<const Key, Value>>>
using HashMap = std::unordered_map<Key, Value, Hash, OpEqual, Allocator>;

template <typename Value, typename Hash = typename Detail::HashAlias<Value>::Type,
          typename OpEqual = typename Detail::OpAlias<Value>::Type, typename Allocator = Memory::STLAllocator<Value>>
using HashSet = std::unordered_set<Value, Hash, OpEqual>;

template <typename Key, typename Value, typename Compare = std::less<Key>,
          typename Allocator = Memory::STLAllocator<std::pair<const Key, Value>>>
using TreeMap = std::map<Key, Value, Compare, Allocator>;

template <typename Value, typename Compare = std::less<Value>, typename Allocator = Memory::STLAllocator<Value>>
using TreeSet = std::set<Value, Compare, Allocator>;
} // namespace TKit
