#pragma once

#include "tkit/container/array.hpp"
#include "tkit/memory/arena_allocator.hpp"

namespace TKit
{
template <typename T> struct ArenaAllocation
{
    static constexpr ArrayType Type = Array_Arena;

    ArenaAllocation() = default;
    ArenaAllocation(const usize capacity, ArenaAllocator *allocator = nullptr) : Allocator(allocator)
    {
        Allocate(capacity);
    }
    ArenaAllocation(ArenaAllocator *allocator) : Allocator(allocator)
    {
    }
    ArenaAllocation(ArenaAllocator *allocator, const usize capacity) : Allocator(allocator)
    {
        Allocate(capacity);
    }

    ArenaAllocation(const ArenaAllocation &) = delete;
    ArenaAllocation(ArenaAllocation &&other)
        : Allocator(other.Allocator), Data(other.Data), Size(other.Size), Capacity(other.Capacity)
    {
        other.Data = nullptr;
        other.Size = 0;
        other.Capacity = 0;
    }

    ArenaAllocation &operator=(const ArenaAllocation &) = delete;
    ArenaAllocation &operator=(ArenaAllocation &&other)
    {
        Allocator = other.Allocator;
        Data = other.Data;
        Size = other.Size;
        Capacity = other.Capacity;
        other.Data = nullptr;
        other.Size = 0;
        other.Capacity = 0;
        return *this;
    }

    void Allocate(const usize capacity)
    {
        if (capacity == 0)
            return;
        TKIT_ASSERT(
            Size == 0,
            "[TOOLKIT][ARENA-ARRAY] Cannot allocate while the array has {} active allocations. Call Clear() first",
            Size);
        TKIT_ASSERT(Capacity == 0, "[TOOLKIT][ARENA-ARRAY] Cannot allocate with an active capacity of {}", Capacity);
        TKIT_ASSERT(!Data,
                    "[TOOLKIT][ARENA-ARRAY] Cannot allocate with an active allocation. In fact, an active allocation "
                    "cannot exist if capacity is 0. Capacity: {}",
                    Capacity);

        if (!Allocator)
            Allocator = Memory::GetArena();
        TKIT_ASSERT(Allocator, "[TOOLKIT][ARENA-ARRAY] Array must have a valid allocator to allocate memory");
        Data = Allocator->Allocate<T>(capacity);
        TKIT_ASSERT(Data, "[TOOLKIT][ARENA-ARRAY] Failed to allocate {} bytes of memory", capacity * sizeof(T));
        Capacity = capacity;
    }

    constexpr usize GetCapacity() const
    {
        return Capacity;
    }

    ArenaAllocator *Allocator = nullptr;
    T *Data = nullptr;
    usize Size = 0;
    usize Capacity = 0;
};
template <typename T> using ArenaArray = Array<T, ArenaAllocation<T>>;
} // namespace TKit
