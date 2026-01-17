#pragma once

#include "tkit/container/array.hpp"

namespace TKit
{
template <typename T> struct DynamicAllocation
{
    static constexpr ArrayType Type = Array_Dynamic;

    DynamicAllocation() = default;
    DynamicAllocation(const usize p_Capacity)
    {
        Allocate(p_Capacity);
    }

    DynamicAllocation(const DynamicAllocation &) = delete;
    DynamicAllocation(DynamicAllocation &&p_Other) : Data(p_Other.Data), Size(p_Other.Size), Capacity(p_Other.Capacity)
    {
        p_Other.Data = nullptr;
        p_Other.Size = 0;
        p_Other.Capacity = 0;
    }

    DynamicAllocation &operator=(const DynamicAllocation &) = delete;
    DynamicAllocation &operator=(DynamicAllocation &&p_Other)
    {

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
            "[TOOLKIT][DYN-ARRAY] Cannot allocate while the array has {} active allocations. Call Clear() first", Size);
        TKIT_ASSERT(Capacity == 0, "[TOOLKIT][DYN-ARRAY] Cannot allocate with an active capacity of {}", Capacity);
        TKIT_ASSERT(!Data,
                    "[TOOLKIT][DYN-ARRAY] Cannot allocate with an active allocation. In fact, an active allocation "
                    "cannot exist if capacity is 0. Capacity: {}",
                    Capacity);

        TKIT_ASSERT(p_Capacity != 0, "[TOOLKIT][DYN-ARRAY] Capacity must be greater than 0");
        Data = static_cast<T *>(Memory::AllocateAligned(p_Capacity * sizeof(T), alignof(T)));
        Capacity = p_Capacity;
    }

    void Deallocate()
    {
        TKIT_ASSERT(Size == 0, "[TOOLKIT][DYN-ARRAY] Cannot deallocate buffer while it is not empty. Size is {}", Size);
        if (Data)
        {
            TKIT_ASSERT(Capacity != 0,
                        "[TOOLKIT][DYN-ARRAY] Capacity cannot be zero if buffer is about to be deallocated");
            Memory::DeallocateAligned(Data);
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
        TKIT_ASSERT(p_Capacity > 0, "[TOOLKIT][DYN-ARRAY] Capacity must be greater than 0");
        TKIT_ASSERT(p_Capacity >= Size, "[TOOLKIT][DYN-ARRAY] Capacity ({}) is smaller than size ({})", p_Capacity,
                    Size);
        T *newData = static_cast<T *>(Memory::AllocateAligned(p_Capacity * sizeof(T), alignof(T)));
        TKIT_ASSERT(newData, "[TOOLKIT][DYN-ARRAY] Failed to allocate {} bytes of memory aligned to {} bytes",
                    p_Capacity * sizeof(T), alignof(T));

        if (Data)
        {
            Tools::MoveConstructFromRange(newData, Data, Data + Size);
            if constexpr (!std::is_trivially_destructible_v<T>)
                Memory::DestructRange(Data, Data + Size);
            Memory::DeallocateAligned(Data);
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

    T *Data = nullptr;
    usize Size = 0;
    usize Capacity = 0;
};
template <typename T> using DynamicArray = Array<T, DynamicAllocation<T>>;
} // namespace TKit
