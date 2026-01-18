#pragma once

#include "tkit/container/array.hpp"
#include "tkit/memory/arena_allocator.hpp"

namespace TKit
{
template <typename T> struct ArenaAllocation
{
    static constexpr ArrayType Type = Array_Arena;

    ArenaAllocation() = default;
    ArenaAllocation(const usize p_Capacity, ArenaAllocator *p_Allocator = nullptr) : Allocator(p_Allocator)
    {
        Allocate(p_Capacity);
    }
    ArenaAllocation(ArenaAllocator *p_Allocator) : Allocator(p_Allocator)
    {
    }
    ArenaAllocation(ArenaAllocator *p_Allocator, const usize p_Capacity) : Allocator(p_Allocator)
    {
        Allocate(p_Capacity);
    }

    ArenaAllocation(const ArenaAllocation &) = delete;
    ArenaAllocation(ArenaAllocation &&p_Other)
        : Allocator(p_Other.Allocator), Data(p_Other.Data), Size(p_Other.Size), Capacity(p_Other.Capacity)
    {
        p_Other.Data = nullptr;
        p_Other.Size = 0;
        p_Other.Capacity = 0;
    }

    ArenaAllocation &operator=(const ArenaAllocation &) = delete;
    ArenaAllocation &operator=(ArenaAllocation &&p_Other)
    {
        Allocator = p_Other.Allocator;
        Data = p_Other.Data;
        Size = p_Other.Size;
        Capacity = p_Other.Capacity;
        p_Other.Data = nullptr;
        p_Other.Size = 0;
        p_Other.Capacity = 0;
        return *this;
    }

    void Allocate(const usize p_Capacity)
    {
        if (p_Capacity == 0)
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
        Data = Allocator->Allocate<T>(p_Capacity);
        Capacity = p_Capacity;
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
