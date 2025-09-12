#include "tkit/core/pch.hpp"
#include "tkit/memory/tier_allocator.hpp"
#include "tkit/utils/bit.hpp"
#include "tkit/utils/logging.hpp"

namespace TKit
{
TierAllocator::Description TierAllocator::CreateDescription(const usize p_MaxAllocation, const usize p_MinAllocation,
                                                            const usize p_Granularity, const f32 p_TierSlotDecay)
{
    TKIT_ASSERT(
        IsPowerOfTwo(p_MaxAllocation) && IsPowerOfTwo(p_MinAllocation) && IsPowerOfTwo(p_Granularity),
        "[TOOLKIT][TIER-ALLOC] All integer arguments must be powers of two when creating a tier allocator description");
    TKIT_ASSERT(p_MinAllocation >= sizeof(void *),
                "[TOOLKIT][TIER-ALLOC] Minimum allocation size must be at least sizeof(void *)");
    TKIT_ASSERT(p_Granularity <= p_MinAllocation,
                "[TOOLKIT][TIER-ALLOC] Granularity must be less or equal than the minimum allocation");
    TKIT_ASSERT(p_Granularity >= 2, "[TOOLKIT][TIER-ALLOC] Granularity cannot be smaller than 2");
    TKIT_ASSERT(p_TierSlotDecay > 0.f && p_TierSlotDecay <= 1.f,
                "[TOOLKIT][TIER-ALLOC] Tier slot decay must be between 0 and 1.");

    Description desc{};
    desc.MaxAllocation = p_MaxAllocation;
    desc.MinAllocation = p_MinAllocation;
    desc.Granularity = p_Granularity;
    desc.TierSlotDecay = p_TierSlotDecay;

    usize size = p_MaxAllocation;
    usize prevAlloc = p_MaxAllocation;
    usize prevSlots = 1;

    const auto nextAlloc = [p_Granularity](const usize p_CurrentAlloc) {
        const usize increment = NextPowerOfTwo(p_CurrentAlloc) / p_Granularity;
        return p_CurrentAlloc - increment;
    };

    usize currentAlloc = nextAlloc(prevAlloc);
    usize currentSlots = 0;

    desc.Tiers.Append(TierInfo{.Size = p_MaxAllocation, .AllocationSize = p_MaxAllocation, .Slots = 1});

    const auto enoughSlots = [&currentSlots, &prevSlots, p_TierSlotDecay]() {
        return static_cast<usize>(p_TierSlotDecay * currentSlots) >= prevSlots;
    };

    while (true)
    {
        const usize psize = size;
        const usize alignment = PrevPowerOfTwo(currentAlloc);
        while (!enoughSlots() || (currentAlloc != p_MinAllocation && size % alignment != 0))
        {
            ++currentSlots;
            size += currentAlloc;
        }
        TierInfo tier{};
        tier.AllocationSize = currentAlloc;
        tier.Size = size - psize;
        TKIT_ASSERT(tier.Size % tier.AllocationSize == 0,
                    "[TOOLKIT][TIER-ALLOC] Tier with size {} is not a perfect fit for the allocation size {}",
                    tier.Size, tier.AllocationSize);

        tier.Slots = tier.Size / tier.AllocationSize;
        desc.Tiers.Append(tier);

        if (currentAlloc == p_MinAllocation)
            break;

        prevAlloc = currentAlloc;
        prevSlots = currentSlots;

        currentAlloc = nextAlloc(prevAlloc);
        currentSlots = 0;
    }
    desc.BufferSize = size;
    return desc;
}
TierAllocator::TierAllocator(const usize p_MaxAllocation, const usize p_MinAllocation, const usize p_Granularity,
                             const f32 p_TierSlotDecay, const usize p_MaxAlignment)
    : TierAllocator(CreateDescription(p_MaxAllocation, p_MinAllocation, p_Granularity, p_TierSlotDecay), p_MaxAlignment)
{
}

TierAllocator::TierAllocator(const Description &p_Description, const usize p_MaxAlignment)
    : m_MinAllocation(p_Description.MinAllocation), m_Granularity(p_Description.Granularity)
{
    TKIT_ASSERT(IsPowerOfTwo(p_MaxAlignment), "[TOOLKIT][TIER-ALLOC] Maximum alignment must be a power of 2");

    m_Buffer = static_cast<std::byte *>(Memory::AllocateAligned(p_Description.BufferSize, p_MaxAlignment));
    TKIT_ASSERT(m_Buffer,
                "[TOOLKIT][TIER-ALLOC] Failed to allocate final allocator buffer of {} bytes aligned to {}. "
                "Consider choosing another parameter combination",
                p_Description.BufferSize, p_MaxAlignment);
    m_BufferSize = p_Description.BufferSize;

    setupMemoryLayout(p_Description, p_MaxAlignment);
}

void TierAllocator::setupMemoryLayout(const Description &p_Description, const usize p_MaxAlignment)
{
    usize size = 0;
    for (const TierInfo &tinfo : p_Description.Tiers)
    {
        Tier tier{};
        tier.Buffer = m_Buffer + size;
        tier.FreeList = reinterpret_cast<Allocation *>(tier.Buffer);
        const usize count = tinfo.Size / tinfo.AllocationSize;

        TKIT_ASSERT(Memory::IsAligned(tier.Buffer, std::min(p_MaxAlignment, PrevPowerOfTwo(tinfo.AllocationSize))),
                    "[TOOLKIT][TIER-ALLOC] Tier with size {} and buffer {} failed alignment check: it is not aligned "
                    "to either the maximum alignment or its previous power of 2",
                    tinfo.Size, static_cast<void *>(tier.Buffer));

        Allocation *next = nullptr;
        for (usize i = count - 1; i < count; --i)
        {
            Allocation *alloc = reinterpret_cast<Allocation *>(tier.Buffer + i * tinfo.AllocationSize);
            alloc->Next = next;
            next = alloc;
        }
        m_Tiers.Append(tier);
        size += tinfo.Size;
    }
}

void *TierAllocator::Allocate(const usize p_Size)
{
    const usize index = getTierIndex(p_Size);
    Tier &tier = m_Tiers[index];
    if (!tier.FreeList)
        return nullptr;

    Allocation *alloc = tier.FreeList;
    tier.FreeList = alloc->Next;
    return alloc;
}

void TierAllocator::Deallocate(void *p_Ptr, const usize p_Size)
{
    TKIT_ASSERT(p_Ptr, "[TOOLKIT][TIER-ALLOC] Cannot deallocate a null pointer");
    TKIT_ASSERT(Belongs(p_Ptr),
                "[TOOLKIT][TIER-ALLOC] Cannot deallocate a pointer that does not belong to the allocator");

    const usize index = getTierIndex(p_Size);
    Tier &tier = m_Tiers[index];
    Allocation *alloc = static_cast<Allocation *>(p_Ptr);
    alloc->Next = tier.FreeList;
    tier.FreeList = alloc;
}

bool TierAllocator::Belongs(const void *p_Ptr) const
{
    const std::byte *ptr = static_cast<const std::byte *>(p_Ptr);
    return ptr >= m_Buffer && ptr < m_Buffer + m_BufferSize;
}

usize TierAllocator::GetBufferSize() const
{
    return m_BufferSize;
}

static usize bitIndex(const usize p_Value)
{
    return static_cast<usize>(std::countr_zero(p_Value));
}

static usize getTierIndex(const usize p_Size, const usize p_MinAllocation, const usize p_Granularity,
                          const usize p_LastIndex)
{
    if (p_Size <= p_MinAllocation)
        return p_LastIndex;

    const usize np2 = NextPowerOfTwo(p_Size);
    const usize increment = np2 / p_Granularity;
    const usize factor = p_Granularity >> 1;
    const usize reference = np2 - p_Size;
    const usize incIndex = bitIndex(increment);

    // Signed code for a bit more correctness, but as final result is guaranteed to not exceed uint max, it is not
    // strictly needed constexpr auto cast = [](const usize p_Value) { return static_cast<ssize>(p_Value); };
    //
    // const ssize offset = cast(bitIndex(p_MinAllocation)) - cast(bitIndex(p_Granularity));
    //
    // const ssize idx = cast(p_LastIndex) + cast(factor) * (offset - cast(incIndex)) + cast(reference / increment);
    // return static_cast<usize>(idx);
    const usize offset = bitIndex(p_MinAllocation) - bitIndex(p_Granularity);

    return p_LastIndex + factor * (offset - incIndex) + reference / increment;
}

usize TierAllocator::Description::GetTierIndex(const usize p_Size)
{
    return TKit::getTierIndex(p_Size, MinAllocation, Granularity, Tiers.GetSize() - 1);
}
usize TierAllocator::getTierIndex(const usize p_Size)
{
    return TKit::getTierIndex(p_Size, m_MinAllocation, m_Granularity, m_Tiers.GetSize() - 1);
}
} // namespace TKit
