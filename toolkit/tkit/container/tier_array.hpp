#pragma once

#include "tkit/container/array.hpp"
#include "tkit/memory/tier_allocator.hpp"

namespace TKit
{
template <typename T> struct TierAllocation
{
    static constexpr ArrayType Type = Array_Tier;

    TierAllocation() = default;
    TierAllocation(TierAllocator *p_Allocator) : Allocator(p_Allocator)
    {
    }
    TierAllocation(TierAllocator *p_Allocator, const usize p_Capacity) : Allocator(p_Allocator)
    {
        Allocate(p_Capacity);
    }

    TierAllocation(const TierAllocation &) = delete;
    TierAllocation(TierAllocation &&p_Other)
        : Allocator(p_Other.Allocator), Data(p_Other.Data), Size(p_Other.Size), Capacity(p_Other.Capacity)
    {
        p_Other.Data = nullptr;
        p_Other.Size = 0;
        p_Other.Capacity = 0;
    }

    TierAllocation &operator=(const TierAllocation &) = delete;
    TierAllocation &operator=(TierAllocation &&p_Other)
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
            "[TOOLKIT][TIER-ARRAY] Cannot allocate while the array has {} active allocations. Call Clear() first",
            Size);
        TKIT_ASSERT(Capacity == 0, "[TOOLKIT][TIER-ARRAY] Cannot allocate with an active capacity of {}", Capacity);
        TKIT_ASSERT(!Data,
                    "[TOOLKIT][TIER-ARRAY] Cannot allocate with an active allocation. In fact, an active allocation "
                    "cannot exist if capacity is 0. Capacity: {}",
                    Capacity);

        TKIT_ASSERT(p_Capacity != 0, "[TOOLKIT][TIER-ARRAY] Capacity must be greater than 0");
        TKIT_ASSERT(Allocator, "[TOOLKIT][TIER-ARRAY] Array must have a valid allocator to allocate memory");
        Data = Allocator->Allocate<T>(p_Capacity);
        Capacity = p_Capacity;
    }

    void Deallocate()
    {
        TKIT_ASSERT(Size == 0, "[TOOLKIT][TIER-ARRAY] Cannot deallocate buffer while it is not empty. Size is {}",
                    Size);
        if (Data)
        {
            TKIT_ASSERT(Capacity != 0,
                        "[TOOLKIT][TIER-ARRAY] Capacity cannot be zero if buffer is about to be deallocated");
            TKIT_ASSERT(Allocator, "[TOOLKIT][TIER-ARRAY] Array must have a valid allocator to deallocate memory");
            Allocator->Deallocate(Data, Capacity);
            Data = nullptr;
            Capacity = 0;
        }
    }
    constexpr usize GetCapacity() const
    {
        return Capacity;
    }

    void GrowCapacityIf(const bool p_ShouldGrow, const usize p_Size)
    {
        if (p_ShouldGrow)
            GrowCapacity(p_Size);
    }
    void ModifyCapacity(const usize p_Capacity)
    {
        using Tools = Container::ArrayTools<T>;
        TKIT_ASSERT(Allocator, "[TOOLKIT][TIER-ARRAY] Array must have a valid allocator to allocate memory");
        TKIT_ASSERT(p_Capacity > 0, "[TOOLKIT][TIER-ARRAY] Capacity must be greater than 0");
        TKIT_ASSERT(p_Capacity >= Size, "[TOOLKIT][TIER-ARRAY] Capacity ({}) is smaller than size ({})", p_Capacity,
                    Size);
        T *newData = Allocator->Allocate<T>(p_Capacity);
        TKIT_ASSERT(newData, "[TOOLKIT][TIER-ARRAY] Failed to allocate {} bytes of memory aligned to {} bytes",
                    p_Capacity * sizeof(T), alignof(T));

        if (Data)
        {
            Tools::MoveConstructFromRange(newData, Data, Data + Size);
            if constexpr (!std::is_trivially_destructible_v<T>)
                Memory::DestructRange(Data, Data + Size);
            Allocator->Deallocate(Data, Capacity);
        }
        Data = newData;
        Capacity = p_Capacity;
    }
    void GrowCapacity(const usize p_Size)
    {
        using Tools = Container::ArrayTools<T>;
        const usize capacity = Tools::GrowthFactor(p_Size);
        ModifyCapacity(capacity);
    }

    TierAllocator *Allocator = nullptr;
    T *Data = nullptr;
    usize Size = 0;
    usize Capacity = 0;
};
template <typename T> using TierArray = Array<T, TierAllocation<T>>;
} // namespace TKit
