#pragma once

#include "tkit/container/array.hpp"
#include "tkit/memory/arena_allocator.hpp"

namespace TKit
{
template <typename T> struct ArenaAllocation
{
    static constexpr bool IsReallocatable = false;
    static constexpr bool IsMovable = true;
    static constexpr bool IsDeallocatable = false;
    static constexpr bool HasAllocator = true;

    using AllocatorType = ArenaAllocator;

    ArenaAllocation() = default;
    ArenaAllocation(ArenaAllocator *p_Allocator, const usize p_Capacity, const usize p_Size = 0)
        : Allocator(p_Allocator), Size(p_Size), Capacity(p_Capacity)
    {
        TKIT_ASSERT(p_Capacity != 0, "[TOOLKIT][ARRAY] Capacity must be greater than 0");
        Data = p_Allocator->Allocate<T>(p_Capacity);
    }

    ArenaAllocation(const ArenaAllocation &) = delete;
    ArenaAllocation(ArenaAllocation &&p_Other) : Data(p_Other.Data), Size(p_Other.Size), Capacity(p_Other.Capacity)
    {
        p_Other.Data = nullptr;
        p_Other.Size = 0;
        p_Other.Capacity = 0;
    }

    ArenaAllocation &operator=(const ArenaAllocation &) = delete;
    ArenaAllocation &operator=(ArenaAllocation &&p_Other)
    {
        Data = p_Other.Data;
        Size = p_Other.Size;
        Capacity = p_Other.Capacity;
        p_Other.Data = nullptr;
        p_Other.Size = 0;
        p_Other.Capacity = 0;
        return *this;
    }

    void Deallocate()
    {
        TKIT_ASSERT(Size == 0, "[TOOLKIT][ARRAY] Cannot deallocate buffer while it is not empty. Size is {}", Size);
        if (Data)
        {
            Memory::DeallocateAligned(Data);
            Data = nullptr;
            Capacity = 0;
        }
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
