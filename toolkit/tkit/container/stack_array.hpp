#pragma once

#include "tkit/container/array.hpp"
#include "tkit/memory/stack_allocator.hpp"

namespace TKit
{
template <typename T> struct StackAllocation
{
    static constexpr ArrayType Type = Array_Stack;

    StackAllocation() = default;
    StackAllocation(StackAllocator *p_Allocator, const usize p_Capacity) : Allocator(p_Allocator)
    {
        Allocate(p_Capacity);
    }

    StackAllocation(const StackAllocation &) = delete;
    StackAllocation(StackAllocation &&p_Other)
        : Allocator(p_Other.Allocator), Data(p_Other.Data), Size(p_Other.Size), Capacity(p_Other.Capacity)
    {
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
        p_Other.Data = nullptr;
        p_Other.Size = 0;
        p_Other.Capacity = 0;
        return *this;
    }

    void Allocate(const usize p_Capacity)
    {
        TKIT_ASSERT(
            Size == 0,
            "[TOOLKIT][STACK-ARRAY] Cannot allocate while the array has {} active allocations. Call Clear() first",
            Size);
        TKIT_ASSERT(Capacity == 0, "[TOOLKIT][STACK-ARRAY] Cannot allocate with an active capacity of {}", Capacity);
        TKIT_ASSERT(!Data,
                    "[TOOLKIT][STACK-ARRAY] Cannot allocate with an active allocation. In fact, an active allocation "
                    "cannot exist if capacity is 0. Capacity: {}",
                    Capacity);

        TKIT_ASSERT(p_Capacity != 0, "[TOOLKIT][STACK-ARRAY] Capacity must be greater than 0");
        TKIT_ASSERT(Allocator, "[TOOLKIT][STACK-ARRAY] Array must have a valid allocator to allocate memory");
        Data = Allocator->Allocate<T>(p_Capacity);
        Capacity = p_Capacity;
    }

    void Deallocate()
    {
        TKIT_ASSERT(Size == 0, "[TOOLKIT][STACK-ARRAY] Cannot deallocate buffer while it is not empty. Size is {}",
                    Size);
        if (Data)
        {
            TKIT_ASSERT(Capacity != 0,
                        "[TOOLKIT][STACK-ARRAY] Capacity cannot be zero if buffer is about to be deallocated");
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
