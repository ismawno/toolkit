#pragma once

#include "tkit/container/array.hpp"
#include "tkit/memory/tier_allocator.hpp"

namespace TKit
{
template <typename T> struct TierAllocation
{
    static constexpr ArrayType Type = Array_Tier;

    TierAllocation() = default;
    TierAllocation(TierAllocator *allocator) : Allocator(allocator)
    {
    }
    TierAllocation(TierAllocator *allocator, const usize capacity) : Allocator(allocator)
    {
        Allocate(capacity);
    }

    TierAllocation(const TierAllocation &) = delete;
    TierAllocation(TierAllocation &&other)
        : Allocator(other.Allocator), Data(other.Data), Size(other.Size), Capacity(other.Capacity)
    {
        other.Data = nullptr;
        other.Size = 0;
        other.Capacity = 0;
    }

    TierAllocation &operator=(const TierAllocation &) = delete;
    TierAllocation &operator=(TierAllocation &&other)
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
            "[TOOLKIT][TIER-ARRAY] Cannot allocate while the array has {} active allocations. Call Clear() first",
            Size);
        TKIT_ASSERT(Capacity == 0, "[TOOLKIT][TIER-ARRAY] Cannot allocate with an active capacity of {}", Capacity);
        TKIT_ASSERT(!Data,
                    "[TOOLKIT][TIER-ARRAY] Cannot allocate with an active allocation. In fact, an active allocation "
                    "cannot exist if capacity is 0. Capacity: {}",
                    Capacity);

        if (!Allocator)
            Allocator = GetTier();
        TKIT_ASSERT(Allocator, "[TOOLKIT][TIER-ARRAY] Array must have a valid allocator to allocate memory");
        Data = Allocator->Allocate<T>(capacity);
        TKIT_ASSERT(Data, "[TOOLKIT][TIER-ARRAY] Failed to allocate {} bytes of memory", capacity * sizeof(T));
        Capacity = capacity;
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

    void GrowCapacityIf(const bool shouldGrow, const usize size)
    {
        if (shouldGrow)
            GrowCapacity(size);
    }
    void ModifyCapacity(const usize capacity)
    {
        using Tools = Container::ArrayTools<T>;
        if (!Data)
        {
            Allocate(capacity);
            return;
        }
        TKIT_ASSERT(Allocator, "[TOOLKIT][TIER-ARRAY] Array must have a valid allocator to allocate memory");
        TKIT_ASSERT(capacity != 0, "[TOOLKIT][TIER-ARRAY] Capacity must be greater than 0");
        TKIT_ASSERT(capacity >= Size, "[TOOLKIT][TIER-ARRAY] Capacity ({}) is smaller than size ({})", capacity, Size);
        T *newData = Allocator->Allocate<T>(capacity);
        TKIT_ASSERT(newData, "[TOOLKIT][TIER-ARRAY] Failed to allocate {} bytes of memory", capacity * sizeof(T));
        Tools::MoveConstructFromRange(newData, Data, Data + Size);
        if constexpr (!std::is_trivially_destructible_v<T>)
            DestructRange(Data, Data + Size);
        Allocator->Deallocate(Data, Capacity);
        Data = newData;
        Capacity = capacity;
    }
    void GrowCapacity(const usize size)
    {
        using Tools = Container::ArrayTools<T>;
        const usize capacity = Tools::GrowthFactor(size);
        ModifyCapacity(capacity);
    }

    TierAllocator *Allocator = nullptr;
    T *Data = nullptr;
    usize Size = 0;
    usize Capacity = 0;
};
template <typename T> using TierArray = Array<T, TierAllocation<T>>;
} // namespace TKit
