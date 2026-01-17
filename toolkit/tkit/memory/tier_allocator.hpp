#ifndef TKIT_ENABLE_TIER_ALLOCATOR
#    error                                                                                                             \
        "[TOOLKIT][TIER-ALLOC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_TIER_ALLOCATOR"
#endif

#include "tkit/container/static_array.hpp"
#include "tkit/memory/memory.hpp"
#include "tkit/utils/non_copyable.hpp"
#include "tkit/utils/debug.hpp"
#include "tkit/utils/limits.hpp"

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
        StaticArray<TierInfo, MaxAllocTiers> Tiers;
        usize BufferSize;
        usize MaxAllocation;
        usize MinAllocation;
        usize Granularity;
        f32 TierSlotDecay;

        usize GetTierIndex(usize p_Size) const;
    };

    /**
     * @brief Create a tier allocator description.
     *
     * The choice of description parameters heavily influences the layout of the tiers and how many allocations each
     * tier supports. The default parameters are suited to create an allocator that supports allocations of up to 1 mb
     * with a reasonable total buffer size. Check the description values to make sure the buffer size has a value that
     * works for you.
     *
     * All integer parameters must be powers of 2. The maximum alignment is provided at construction. Every allocation
     * is guaranteed to be aligned to the maximum alignment or its natural alignment, so the allocator will respect
     * alignment requirements (including the ones set by `alignas`) up to the specified maximum alignment.
     *
     * @param p_MaxAllocation The maximum allocation size the allocator will support. This also equals to the size of
     * the first tier (which is the one with the largest allocation size), meaning only one allocation of
     * `p_MaxAllocation` bytes can be made.
     *
     * @param p_MinAllocation The minimum allowed allocation. Allocation requests smaller than this size will round up
     * to `p_MinAllocation`. It can never be smaller than `sizeof(void *)`.
     *
     * @param p_Granularity It controls how the size difference between tiers evolves, such that the difference between
     * the allocation sizes of tiers i and i + 1 is the next power of 2 from the allocation size i, divided by the
     * granularity. A small granularity causes tier sizes to shrink (remember, tiers are built from biggest to smallest)
     * fast in between tiers, reaching `p_MinAllocation` from `p_MaxAllocation` quicker and thus resulting in a smaller
     * total buffer size, but fragmentation risk is higher. Bigger granularities prevent fragmentation but cause the
     * total buffer size to explode very fast. A granularity of 2 for instance means that tiers always double their
     * capacity with respect the previous one. It cannot be greater than `p_MinAllocation`.
     *
     * @param p_TierSlotDecay A value between 0 and 1 that controls how the amount of slots scales when creating tiers
     * with smaller allocation sizes. A tier with index i + 1 will have at least the amount of slots tier i has divided
     * by this value. The tier with index 0 has always exactly one slot. Setting this value too low may cause the buffer
     * size to explode.
     */
    static Description CreateDescription(usize p_MaxAllocation, usize p_MinAllocation = sizeof(void *),
                                         usize p_Granularity = 4, f32 p_TierSlotDecay = 0.9f);

    explicit TierAllocator(usize p_MaxAllocation, usize p_MinAllocation = sizeof(void *), usize p_Granularity = 4,
                           f32 p_TierSlotDecay = 0.9f, usize p_MaxAlignment = 64);
    explicit TierAllocator(const Description &p_Description, usize p_MaxAlignment = 64);

    ~TierAllocator();

    TierAllocator(TierAllocator &&p_Other);
    TierAllocator &operator=(TierAllocator &&p_Other);

    void *Allocate(usize p_Size);
    void Deallocate(void *p_Ptr, usize p_Size);

    template <typename T> T *Allocate(const usize p_Count = 1)
    {
        T *ptr = static_cast<T *>(Allocate(p_Count * sizeof(T)));
        TKIT_ASSERT(!ptr || Memory::IsAligned(ptr, alignof(T)),
                    "[TOOLKIT][BLOCK-ALLOC] Type T has stronger memory alignment requirements than specified. Bump the "
                    "alignment of the allocator or prevent using it to allocate objects of such type");
        return ptr;
    }

    template <typename T, typename... Args> T *Create(Args &&...p_Args)
    {
        T *ptr = Allocate<T>();
        if (!ptr)
            return nullptr;
        return Memory::Construct(ptr, std::forward<Args>(p_Args)...);
    }

    template <typename T, typename... Args> T *NCreate(const usize p_Count, Args &&...p_Args)
    {
        T *ptr = Allocate<T>(p_Count);
        if (!ptr)
            return nullptr;
        Memory::ConstructRange(ptr, ptr + p_Count, std::forward<Args>(p_Args)...);
        return ptr;
    }

    template <typename T> void Destroy(T *p_Ptr, const usize p_Count = 1)
    {
        TKIT_ASSERT(p_Ptr, "[TOOLKIT][TIER-ALLOC] Cannot deallocate a null pointer");
        TKIT_ASSERT(Belongs(p_Ptr),
                    "[TOOLKIT][TIER-ALLOC] Cannot deallocate a pointer that does not belong to the allocator");
        if constexpr (!std::is_trivially_destructible_v<T>)
            for (usize i = 0; i < p_Count; ++i)
                p_Ptr[i].~T();

        Deallocate(p_Ptr, p_Count * sizeof(T));
    }

    /**
     * @brief Check if a pointer belongs to the tier allocator.
     *
     * @note This is a simple check to see if the provided pointer lies within the boundaries of the buffer. It will not
     * be able to determine if the pointer is currently allocated or free.
     *
     * @param p_Ptr The pointer to check.
     * @return Whether the pointer belongs to the tier allocator.
     */
    bool Belongs(const void *p_Ptr) const
    {
        const std::byte *ptr = static_cast<const std::byte *>(p_Ptr);
        return ptr >= m_Buffer && ptr < m_Buffer + m_BufferSize;
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
    };

    usize getTierIndex(usize p_Size) const;
#ifdef TKIT_ENABLE_ASSERTS
    void setupMemoryLayout(const Description &p_Description, usize p_MaxAlignment);
#else
    void setupMemoryLayout(const Description &p_Description);
#endif
    void deallocateBuffer();

    StaticArray<Tier, MaxAllocTiers> m_Tiers{};
    std::byte *m_Buffer;
    usize m_BufferSize;
    usize m_MinAllocation;
    usize m_Granularity;
};
TKIT_COMPILER_WARNING_IGNORE_POP()
} // namespace TKit
