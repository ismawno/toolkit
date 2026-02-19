#pragma once

#ifndef TKIT_ENABLE_TIER_ALLOCATOR
#    error                                                                                                             \
        "[TOOLKIT][TIER-ALLOC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_TIER_ALLOCATOR"
#endif

#include "tkit/container/arena_array.hpp"
#include "tkit/memory/memory.hpp"
#include "tkit/utils/non_copyable.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit
{
/**
 * @brief A fast general purpose allocator consisting of multiple tiers that allow for different, fixed allocation
 * sizes.
 *
 * Each tier is a memory region with a different size. All tiers combined form the single underlying general buffer this
 * allocator owns. Every tier has an associated, fixed allocation size, such that every tier supports a specific number
 * of available slots of that size. The tier size is always a perfect fit for the allocation size (slot size). The
 * amount of slots each tier holds is not defined directly.
 *
 * Tiers are built from biggest to smallest allocation sizes, and so they are sorted in that manner -> lower tier
 * indices reference bigger allocation size tiers. Note that a low index total tier size may be smaller than a high
 * index total tier size.
 *
 */
TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_MSVC_WARNING_IGNORE(4324)
class alignas(TKIT_CACHE_LINE_SIZE) TierAllocator
{
    TKIT_NON_COPYABLE(TierAllocator)
  public:
    struct TierInfo
    {
        usize Size;
        usize AllocationSize;
        usize Slots;
    };
    struct Description
    {
        Description(ArenaAllocator *allocator, const usize maxTiers) : Tiers(allocator, maxTiers)
        {
        }
        ArenaArray<TierInfo> Tiers;
        usize BufferSize;
        usize MaxAllocation;
        usize MinAllocation;
        usize Granularity;
        f32 TierSlotDecay;

        usize GetTierIndex(usize size) const;
    };

    /**
     * @brief Create a tier allocator description.
     *
     * The choice of description parameters heavily influences the layout of the tiers and how many allocations each
     * tier supports. The default parameters are suited to create an allocator that supports allocations of up to 1 mb
     * with a reasonable total buffer size. Check the description values to make sure the buffer size has a value that
     * works for you.
     *
     * All integer parameters (except for `maxTiers`) must be powers of 2. The maximum alignment is provided at
     * construction. Every allocation is guaranteed to be aligned to the maximum alignment or its natural alignment, so
     * the allocator will respect alignment requirements (including the ones set by `alignas`) up to the specified
     * maximum alignment.
     *
     * @param maxAllocation The maximum allocation size the allocator will support. This also equals to the size of
     * the first tier (which is the one with the largest allocation size), meaning only one allocation of
     * `maxAllocation` bytes can be made.
     *
     * @param granularity It controls how the size difference between tiers evolves, such that the difference between
     * the allocation sizes of tiers i and i + 1 is the next power of 2 from the allocation size i, divided by the
     * granularity. A small granularity causes tier sizes to shrink (remember, tiers are built from biggest to smallest)
     * fast in between tiers, reaching `minAllocation` from `maxAllocation` quicker and thus resulting in a smaller
     * total buffer size, but fragmentation risk is higher. Bigger granularities prevent fragmentation but cause the
     * total buffer size to explode very fast. A granularity of 2 for instance means that tiers always double their
     * capacity with respect the previous one. It cannot be greater than `minAllocation`.
     *
     * @param tierSlotDecay A value between 0 and 1 that controls how the amount of slots scales when creating tiers
     * with smaller allocation sizes. A tier with index i + 1 will have at least the amount of slots tier i has divided
     * by this value. The tier with index 0 has always exactly one slot. Setting this value too low may cause the buffer
     * size to explode.
     *
     * @param minAllocation The minimum allowed allocation. Allocation requests smaller than this size will round up
     * to `minAllocation`. It can never be smaller than `sizeof(void *)`. If zero, it will default to `granularity *
     * sizeof(void *) / 2`
     *
     */
    static Description CreateDescription(ArenaAllocator *allocator, usize maxTiers, usize maxAllocation,
                                         usize granularity = 4, f32 tierSlotDecay = 0.9f, usize minAllocation = 0);
    static Description CreateDescription(usize maxTiers, usize maxAllocation, usize granularity = 4,
                                         f32 tierSlotDecay = 0.9f, usize minAllocation = 0);

    explicit TierAllocator(ArenaAllocator *allocator, usize maxTiers, usize maxAllocation, usize granularity = 4,
                           f32 tierSlotDecay = 0.9f, usize maxAlignment = 64, usize minAllocation = 0);
    explicit TierAllocator(const Description &description, usize maxAlignment = 64);

    explicit TierAllocator(usize maxTiers, usize maxAllocation, usize granularity = 4, f32 tierSlotDecay = 0.9f,
                           usize maxAlignment = 64, usize minAllocation = 0);

    ~TierAllocator();

    TierAllocator(TierAllocator &&other);
    TierAllocator &operator=(TierAllocator &&other);

    void *Allocate(usize size);
    void Deallocate(void *ptr, usize size);

    template <typename T> T *Allocate(const usize count = 1)
    {
        T *ptr = static_cast<T *>(Allocate(count * sizeof(T)));
        TKIT_ASSERT(!ptr || IsAligned(ptr, alignof(T)),
                    "[TOOLKIT][TIER-ALLOC] Type T has stronger memory alignment requirements than specified. Bump the "
                    "alignment of the allocator or prevent using it to allocate objects of such type");
        return ptr;
    }

    template <typename T>
        requires(!std::same_as<T, void>)
    void Deallocate(T *ptr, const usize count = 1)
    {
        Deallocate(static_cast<void *>(ptr), count * sizeof(T));
    }

    template <typename T, typename... Args> T *Create(Args &&...args)
    {
        T *ptr = Allocate<T>();
        if (!ptr)
            return nullptr;
        return Construct(ptr, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args> T *NCreate(const usize count, Args &&...args)
    {
        T *ptr = Allocate<T>(count);
        if (!ptr)
            return nullptr;
        ConstructRange(ptr, ptr + count, std::forward<Args>(args)...);
        return ptr;
    }

    template <typename T>
        requires(!std::same_as<T, void>)
    constexpr void Destroy(T *ptr)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            ptr->~T();
        Deallocate(ptr);
    }

    template <typename T> void NDestroy(T *ptr, const usize count)
    {
        TKIT_ASSERT(ptr, "[TOOLKIT][TIER-ALLOC] Cannot deallocate a null pointer");
        TKIT_ASSERT(Belongs(ptr),
                    "[TOOLKIT][TIER-ALLOC] Cannot deallocate a pointer that does not belong to the allocator");
        if constexpr (!std::is_trivially_destructible_v<T>)
            for (usize i = 0; i < count; ++i)
                ptr[i].~T();
        Deallocate(ptr, count);
    }

    /**
     * @brief Check if a pointer belongs to the tier allocator.
     *
     * @note This is a simple check to see if the provided pointer lies within the boundaries of the buffer. It will not
     * be able to determine if the pointer is currently allocated or free.
     *
     * @param ptr The pointer to check.
     * @return Whether the pointer belongs to the tier allocator.
     */
    bool Belongs(const void *ptr) const
    {
        const std::byte *bptr = static_cast<const std::byte *>(ptr);
        return bptr >= m_Buffer && bptr < m_Buffer + m_BufferSize;
    }

    usize GetBufferSize() const
    {
        return m_BufferSize;
    }

  private:
    struct Allocation
    {
        Allocation *Next;
    };

    struct Tier
    {
        std::byte *Buffer;
        Allocation *FreeList;
#ifdef TKIT_ENABLE_ASSERTS
        usize Allocations = 0;
        usize Deallocations = 0;
        usize Slots = 0;
#endif
    };

    usize getTierIndex(usize size) const;
#ifdef TKIT_ENABLE_ASSERTS
    void setupMemoryLayout(const Description &description, usize maxAlignment);
#else
    void setupMemoryLayout(const Description &description);
#endif
    void deallocateBuffer();

    ArenaArray<Tier> m_Tiers;
    std::byte *m_Buffer;
    usize m_BufferSize;
    usize m_MinAllocation;
    usize m_Granularity;
#ifdef TKIT_ENABLE_ASSERTS
    usize m_MaxAllocation;
#endif
};
TKIT_COMPILER_WARNING_IGNORE_POP()
} // namespace TKit
