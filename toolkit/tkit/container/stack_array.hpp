#pragma once

#include "tkit/container/array.hpp"
#include "tkit/memory/stack_allocator.hpp"

namespace TKit
{
template <typename T> struct StackAllocation
{
    static constexpr ArrayType Type = Array_Stack;

    StackAllocation() = default;
    StackAllocation(StackAllocator *allocator) : Allocator(allocator)
    {
    }
    StackAllocation(StackAllocator *allocator, const usize capacity) : Allocator(allocator)
    {
        Allocate(capacity);
    }

    StackAllocation(const StackAllocation &) = delete;
    StackAllocation(StackAllocation &&other)
        : Allocator(other.Allocator), Data(other.Data), Size(other.Size), Capacity(other.Capacity)
    {
        other.Data = nullptr;
        other.Size = 0;
        other.Capacity = 0;
    }

    StackAllocation &operator=(const StackAllocation &) = delete;
    StackAllocation &operator=(StackAllocation &&other)
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
            "[TOOLKIT][STACK-ARRAY] Cannot allocate while the array has {} active allocations. Call Clear() first",
            Size);
        TKIT_ASSERT(Capacity == 0, "[TOOLKIT][STACK-ARRAY] Cannot allocate with an active capacity of {}", Capacity);
        TKIT_ASSERT(!Data,
                    "[TOOLKIT][STACK-ARRAY] Cannot allocate with an active allocation. In fact, an active allocation "
                    "cannot exist if capacity is 0. Capacity: {}",
                    Capacity);

        if (!Allocator)
            Allocator = Memory::GetStack();
        TKIT_ASSERT(Allocator, "[TOOLKIT][STACK-ARRAY] Array must have a valid allocator to allocate memory");
        Data = Allocator->Allocate<T>(capacity);
        TKIT_ASSERT(Data, "[TOOLKIT][STACK-ARRAY] Failed to allocate {} bytes of memory", capacity * sizeof(T));
        Capacity = capacity;
    }

    void Deallocate()
    {
        TKIT_ASSERT(Size == 0, "[TOOLKIT][STACK-ARRAY] Cannot deallocate buffer while it is not empty. Size is {}",
                    Size);
        if (Data)
        {
            TKIT_ASSERT(Capacity != 0,
                        "[TOOLKIT][STACK-ARRAY] Capacity cannot be zero if buffer is about to be deallocated");
            TKIT_ASSERT(Allocator, "[TOOLKIT][STACK-ARRAY] Array must have a valid allocator to deallocate memory");
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
