#pragma once

#include "tkit/container/array.hpp"

namespace TKit
{
template <typename T> struct DynamicAllocation
{
    static constexpr ArrayType Type = Array_Dynamic;

    DynamicAllocation() = default;
    DynamicAllocation(const usize capacity)
    {
        Allocate(capacity);
    }

    DynamicAllocation(const DynamicAllocation &) = delete;
    DynamicAllocation(DynamicAllocation &&other) : Data(other.Data), Size(other.Size), Capacity(other.Capacity)
    {
        other.Data = nullptr;
        other.Size = 0;
        other.Capacity = 0;
    }

    DynamicAllocation &operator=(const DynamicAllocation &) = delete;
    DynamicAllocation &operator=(DynamicAllocation &&other)
    {

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
            "[TOOLKIT][DYN-ARRAY] Cannot allocate while the array has {} active allocations. Call Clear() first", Size);
        TKIT_ASSERT(Capacity == 0, "[TOOLKIT][DYN-ARRAY] Cannot allocate with an active capacity of {}", Capacity);
        TKIT_ASSERT(!Data,
                    "[TOOLKIT][DYN-ARRAY] Cannot allocate with an active allocation. In fact, an active allocation "
                    "cannot exist if capacity is 0. Capacity: {}",
                    Capacity);

        Data = static_cast<T *>(Memory::AllocateAligned(capacity * sizeof(T), alignof(T)));
        TKIT_ASSERT(Data, "[TOOLKIT][DYN-ARRAY] Failed to allocate {} bytes of memory aligned to {} bytes",
                    capacity * sizeof(T), alignof(T));
        Capacity = capacity;
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

    void GrowCapacityIf(const bool shouldGrow, const usize size)
    {
        if (shouldGrow)
            GrowCapacity(size);
    }
    void ModifyCapacity(const usize capacity)
    {
        using Tools = Container::ArrayTools<T>;
        TKIT_ASSERT(capacity != 0, "[TOOLKIT][DYN-ARRAY] Capacity must be greater than 0");
        TKIT_ASSERT(capacity >= Size, "[TOOLKIT][DYN-ARRAY] Capacity ({}) is smaller than size ({})", capacity, Size);
        T *newData = static_cast<T *>(Memory::AllocateAligned(capacity * sizeof(T), alignof(T)));
        TKIT_ASSERT(newData, "[TOOLKIT][DYN-ARRAY] Failed to allocate {} bytes of memory aligned to {} bytes",
                    capacity * sizeof(T), alignof(T));

        if (Data)
        {
            Tools::MoveConstructFromRange(newData, Data, Data + Size);
            if constexpr (!std::is_trivially_destructible_v<T>)
                Memory::DestructRange(Data, Data + Size);
            Memory::DeallocateAligned(Data);
        }
        Data = newData;
        Capacity = capacity;
    }
    void GrowCapacity(const usize size)
    {
        using Tools = Container::ArrayTools<T>;
        const usize capacity = Tools::GrowthFactor(size);
        ModifyCapacity(capacity);
    }

    T *Data = nullptr;
    usize Size = 0;
    usize Capacity = 0;
};
template <typename T> using DynamicArray = Array<T, DynamicAllocation<T>>;
} // namespace TKit
