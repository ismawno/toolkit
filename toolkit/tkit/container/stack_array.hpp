#pragma once

#include "tkit/container/array.hpp"
#include "tkit/memory/stack_allocator.hpp"

namespace TKit
{
template <typename T> struct StackAllocation
{
    static constexpr bool IsDeallocatable = true;
    static constexpr bool IsReallocatable = false;
    static constexpr bool IsMovable = true;
    static constexpr bool HasAllocator = true;

    using AllocatorType = StackAllocator;

    StackAllocation() = default;
    StackAllocation(StackAllocator *p_Allocator, const usize p_Capacity, const usize p_Size = 0)
        : Allocator(p_Allocator), Size(p_Size), Capacity(p_Capacity)
    {
        Allocate();
    }

    StackAllocation(const StackAllocation &) = delete;
    StackAllocation(StackAllocation &&p_Other)
        : Allocator(p_Other.Allocator), Data(p_Other.Data), Size(p_Other.Size), Capacity(p_Other.Capacity)
    {
        p_Other.Allocator = nullptr;
        p_Other.Data = nullptr;
        p_Other.Size = 0;
        p_Other.Capacity = 0;
    }

    StackAllocation &operator=(const StackAllocation &) = delete;
    StackAllocation &operator=(StackAllocation &&p_Other)
    {
        Allocator = p_Other.Allocator;
        Data = p_Other.Data;
        Size = p_Other.Size;
        Capacity = p_Other.Capacity;
        p_Other.Allocator = nullptr;
        p_Other.Data = nullptr;
        p_Other.Size = 0;
        p_Other.Capacity = 0;
        return *this;
    }

    void Allocate()
    {
        TKIT_ASSERT(Capacity != 0, "[TOOLKIT][ARRAY] Capacity must be greater than 0");
        Data = Allocator->Allocate<T>(Capacity);
    }

    void Deallocate()
    {
        TKIT_ASSERT(Size == 0, "[TOOLKIT][ARRAY] Cannot deallocate buffer while it is not empty. Size is {}", Size);
        if (Data)
        {
            Allocator->Deallocate(Data, Capacity);
            Data = nullptr;
            Capacity = 0;
        }
    }
    constexpr usize GetCapacity() const
    {
        return Capacity;
    }

    StackAllocator *Allocator = nullptr;
    T *Data = nullptr;
    usize Size = 0;
    usize Capacity = 0;
};
template <typename T> using StackArray = Array<T, StackAllocation<T>>;
} // namespace TKit
