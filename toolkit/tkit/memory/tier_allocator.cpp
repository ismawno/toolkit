#include "tkit/core/pch.hpp"
#include "tkit/memory/tier_allocator.hpp"
#include "tkit/utils/bit.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit
{
static usize bitIndex(const usize value)
{
    return static_cast<usize>(std::countr_zero(value));
}
static usize getTierIndex(const usize size, const usize minAllocation, const usize granularity, const usize lastIndex)
{
    if (size <= minAllocation)
        return lastIndex;

    const usize np2 = NextPowerOfTwo(size);

    const usize grIndex = bitIndex(granularity);
    const usize incIndex = bitIndex(np2 >> grIndex);
    const usize reference = np2 - size;

    // Signed code for a bit more correctness, but as final result is guaranteed to not exceed uint max, it is not
    // strictly needed constexpr auto cast = [](const usize value) { return static_cast<ssize>(value); };
    //
    // const ssize offset = cast(bitIndex(minAllocation)) - cast(bitIndex(granularity));
    //
    // const ssize idx = cast(lastIndex) + cast(factor) * (offset - cast(incIndex)) + cast(reference / increment);
    // return static_cast<usize>(idx);
    const usize offset = bitIndex(minAllocation) - grIndex;

    return lastIndex + ((offset - incIndex) << (grIndex - 1)) + (reference >> incIndex);
}
TierAllocator::Description TierAllocator::CreateDescription(const usize maxTiers, const usize maxAllocation,
                                                            const usize minAllocation, const usize granularity,
                                                            const f32 tierSlotDecay)
{
    return CreateDescription(TKit::GetArena(), maxTiers, maxAllocation, minAllocation, granularity, tierSlotDecay);
}
TierAllocator::Description TierAllocator::CreateDescription(ArenaAllocator *allocator, const usize maxTiers,
                                                            const usize maxAllocation, const usize minAllocation,
                                                            const usize granularity, const f32 tierSlotDecay)
{
    TKIT_ASSERT(IsPowerOfTwo(maxAllocation) && IsPowerOfTwo(minAllocation) && IsPowerOfTwo(granularity),
                "[TOOLKIT][TIER-ALLOC] All integer arguments must be powers of two when creating a tier allocator "
                "description, but the values where {}, {} and {}",
                maxAllocation, minAllocation, granularity);
    TKIT_ASSERT(granularity <= minAllocation,
                "[TOOLKIT][TIER-ALLOC] Granularity ({}) must be less or equal than the minimum allocation ({})",
                granularity, minAllocation);
    TKIT_ASSERT(granularity >= 2, "[TOOLKIT][TIER-ALLOC] Granularity cannot be smaller than 2, but its value was {}",
                granularity);
    TKIT_ASSERT(tierSlotDecay > 0.f && tierSlotDecay <= 1.f,
                "[TOOLKIT][TIER-ALLOC] Tier slot decay must be between 0.0 and 1.0, but its value was {}",
                tierSlotDecay);
    TKIT_ASSERT(
        2 * minAllocation >= sizeof(void *) * granularity,
        "[TOOLKIT][TIER-ALLOC] The minimum allocation must at least be granularity * sizeof(void *) / 2 = {}, but "
        "passed value was {}",
        granularity * sizeof(void *) / 2, minAllocation);

    Description desc{allocator, maxTiers};
    desc.MaxAllocation = maxAllocation;
    desc.MinAllocation = minAllocation;
    desc.Granularity = granularity;
    desc.TierSlotDecay = tierSlotDecay;

    usize size = maxAllocation;
    usize prevAlloc = maxAllocation;
    usize prevSlots = 1;

    const auto nextAlloc = [granularity](const usize currentAlloc) {
        const usize increment = NextPowerOfTwo(currentAlloc) / granularity;
        TKIT_ASSERT(increment % sizeof(void *) == 0,
                    "[TOOLKIT][TIER-ALLOC] Increments in memory between tiers must all be divisible by sizeof(void *) "
                    "= {}, but found an increment of {}. To avoid this error, ensure that minAllocation >= granularity "
                    "* sizeof(void *)",
                    sizeof(void *), increment);
        return currentAlloc - increment;
    };

    usize currentAlloc = nextAlloc(prevAlloc);
    usize currentSlots = 0;

    desc.Tiers.Append(TierInfo{.Size = maxAllocation, .AllocationSize = maxAllocation, .Slots = 1});

    const auto enoughSlots = [&currentSlots, &prevSlots, tierSlotDecay]() {
        return static_cast<usize>(tierSlotDecay * currentSlots) >= prevSlots;
    };

    for (;;)
    {
        const usize psize = size;
        const usize alignment = PrevPowerOfTwo(currentAlloc);
        while (!enoughSlots() || (currentAlloc != minAllocation && size % alignment != 0))
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

        if (currentAlloc == minAllocation)
            break;

        prevAlloc = currentAlloc;
        prevSlots = currentSlots;

        currentAlloc = nextAlloc(prevAlloc);
        currentSlots = 0;
    }
    desc.BufferSize = size;
#ifdef TKIT_ENABLE_ASSERTS
    const auto slowIndex = [&](const usize size) {
        for (usize i = desc.Tiers.GetSize() - 1; i < desc.Tiers.GetSize(); --i)
            if (desc.Tiers[i].AllocationSize >= size)
                return i;
        return desc.Tiers.GetSize();
    };
    for (usize mem = minAllocation; mem <= maxAllocation; ++mem)
    {
        const usize index = TKit::getTierIndex(mem, minAllocation, granularity, desc.Tiers.GetSize() - 1);
        TKIT_ASSERT(desc.Tiers[index].AllocationSize >= mem,
                    "[TOOLKIT][TIER-ALLOC] Allocator is malformed. Found a size of {:L} being assigned a tier index of "
                    "{} with a smaller allocation size of {:L}",
                    mem, index, desc.Tiers[index].AllocationSize);
        TKIT_ASSERT(index == desc.Tiers.GetSize() - 1 || desc.Tiers[index + 1].AllocationSize < mem,
                    "[TOOLKIT][TIER-ALLOC] Allocator is malformed. Found a size of {:L} being assigned a tier index of "
                    "{} with an allocation size of {:L}, but tier index {} has a big enough allocation size of {:L}",
                    mem, index, desc.Tiers[index].AllocationSize, index + 1, desc.Tiers[index + 1].AllocationSize);
        const usize sindex = slowIndex(mem);
        TKIT_ASSERT(sindex == index,
                    "[TOOLKIT][TIER-ALLOC] Allocator is malformed. Brute forced tier index discovery of {} for a size "
                    "of {:L} bytes, while the fast approach computed {}",
                    sindex, mem, index);
    }
#endif
    return desc;
}
TierAllocator::TierAllocator(const usize maxTiers, const usize maxAllocation, const usize minAllocation,
                             const usize granularity, const f32 tierSlotDecay, const usize maxAlignment)
    : TierAllocator(TKit::GetArena(), maxTiers, maxAllocation, minAllocation, granularity, tierSlotDecay, maxAlignment)
{
}
TierAllocator::TierAllocator(ArenaAllocator *allocator, const usize maxTiers, const usize maxAllocation,
                             const usize minAllocation, const usize granularity, const f32 tierSlotDecay,
                             const usize maxAlignment)
    : TierAllocator(CreateDescription(allocator, maxTiers, maxAllocation, minAllocation, granularity, tierSlotDecay),
                    maxAlignment)
{
}

TierAllocator::TierAllocator(const Description &description, const usize maxAlignment)
    : m_Tiers(description.Tiers.GetAllocator(), description.Tiers.GetCapacity()),
      m_MinAllocation(description.MinAllocation), m_Granularity(description.Granularity)
{
#ifdef TKIT_ENABLE_ASSERTS
    m_MaxAllocation = description.MaxAllocation;
#endif
    TKIT_ASSERT(IsPowerOfTwo(maxAlignment),
                "[TOOLKIT][TIER-ALLOC] Maximum alignment must be a power of 2, but {} is not", maxAlignment);

    m_Buffer = static_cast<std::byte *>(AllocateAligned(description.BufferSize, maxAlignment));
    TKIT_ASSERT(m_Buffer,
                "[TOOLKIT][TIER-ALLOC] Failed to allocate final allocator buffer of {:L} bytes aligned to {}. "
                "Consider choosing another parameter combination",
                description.BufferSize, maxAlignment);
    m_BufferSize = description.BufferSize;

#ifdef TKIT_ENABLE_ASSERTS
    setupMemoryLayout(description, maxAlignment);
#else
    setupMemoryLayout(description);
#endif
}

TierAllocator::~TierAllocator()
{
    deallocateBuffer();
}

TierAllocator::TierAllocator(TierAllocator &&other)
    : m_Tiers(std::move(other.m_Tiers)), m_Buffer(other.m_Buffer), m_BufferSize(other.m_BufferSize),
      m_MinAllocation(other.m_MinAllocation), m_Granularity(other.m_Granularity)
{
    other.m_Tiers.Clear();
    other.m_Buffer = nullptr;
    other.m_BufferSize = 0;
    other.m_MinAllocation = 0;
    other.m_Granularity = 0;
}

TierAllocator &TierAllocator::operator=(TierAllocator &&other)
{
    if (this != &other)
    {
        deallocateBuffer();
        m_Tiers = std::move(other.m_Tiers);
        m_Buffer = other.m_Buffer;
        m_BufferSize = other.m_BufferSize;
        m_MinAllocation = other.m_MinAllocation;
        m_Granularity = other.m_Granularity;

        other.m_Tiers.Clear();
        other.m_Buffer = nullptr;
        other.m_BufferSize = 0;
        other.m_MinAllocation = 0;
        other.m_Granularity = 0;
    }
    return *this;
}

#ifdef TKIT_ENABLE_ASSERTS
void TierAllocator::setupMemoryLayout(const Description &description, const usize maxAlignment)
#else
void TierAllocator::setupMemoryLayout(const Description &description)
#endif
{
    usize size = 0;
    for (const TierInfo &tinfo : description.Tiers)
    {
        Tier tier{};
        tier.Buffer = m_Buffer + size;
        tier.FreeList = reinterpret_cast<Allocation *>(tier.Buffer);
        const usize count = tinfo.Size / tinfo.AllocationSize;

        TKIT_ASSERT(IsAligned(tier.Buffer, std::min(maxAlignment, PrevPowerOfTwo(tinfo.AllocationSize))),
                    "[TOOLKIT][TIER-ALLOC] Tier with size {:L} and buffer {} failed alignment check: it is not aligned "
                    "to either the maximum alignment ({}) or its previous power of 2 ({})",
                    tinfo.Size, static_cast<void *>(tier.Buffer), maxAlignment, PrevPowerOfTwo(tinfo.AllocationSize));

        Allocation *next = nullptr;
        for (usize i = count - 1; i < count; --i)
        {
            Allocation *alloc = reinterpret_cast<Allocation *>(tier.Buffer + i * tinfo.AllocationSize);
            TKIT_ASSERT(IsAligned(alloc, alignof(Allocation)),
                        "[TOOLKIT][TIER-ALLOC] Allocation landed in a memory region where its alignment of {} is not "
                        "respected. This happened when using an allocation size of {:L}",
                        alignof(Allocation), tinfo.AllocationSize);
            alloc->Next = next;
            next = alloc;
        }
#ifdef TKIT_ENABLE_ASSERTS
        tier.Slots = tinfo.Slots;
#endif
        m_Tiers.Append(tier);
        size += tinfo.Size;
    }
}

void TierAllocator::deallocateBuffer()
{
    if (m_Buffer)
        DeallocateAligned(m_Buffer);
}

void *TierAllocator::Allocate(const usize size)
{
    TKIT_ASSERT(size <= m_MaxAllocation,
                "[TOOLKIT][TIER-ALLOC] Allocation of size {:L} bytes exceeds max allocation size of {:L}", size,
                m_MaxAllocation);
    const usize index = getTierIndex(size);
    Tier &tier = m_Tiers[index];
    if (!tier.FreeList)
    {
        TKIT_LOG_WARNING("[TOOLKIT][TIER-ALLOC] Allocator ran out of slots when trying to perform an allocation for "
                         "tier index {} and size {:L}",
                         index, size);
        return nullptr;
    }
    TKIT_ASSERT(
        (++tier.Allocations - tier.Deallocations) <= tier.Slots,
        "[TOOLKIT][TIER-ALLOC] Allocator is malformed. Tier of index {} (with allocation of size {:L}) exceeded slots "
        "(allocations - deallocations) = ({} - {}) = {} > slots = {}, but allocator did not attempt to return nullptr",
        index, size, tier.Allocations, tier.Deallocations, tier.Allocations - tier.Deallocations, tier.Slots);

    Allocation *alloc = tier.FreeList;
    tier.FreeList = alloc->Next;
    return alloc;
}

void TierAllocator::Deallocate(void *ptr, const usize size)
{
    TKIT_ASSERT(ptr, "[TOOLKIT][TIER-ALLOC] Cannot deallocate a null pointer");
    TKIT_ASSERT(Belongs(ptr),
                "[TOOLKIT][TIER-ALLOC] Cannot deallocate a pointer that does not belong to the allocator");

    const usize index = getTierIndex(size);
    Tier &tier = m_Tiers[index];
    TKIT_ASSERT(tier.Allocations >= ++tier.Deallocations,
                "[TOOLKIT][TIER-ALLOC] Attempting to deallocate more times than the amount of active alocations there "
                "are for the tier index {} and size {:L}, with {} allocations and {} deallocations",
                index, size, tier.Allocations, tier.Deallocations);

    Allocation *alloc = static_cast<Allocation *>(ptr);
    alloc->Next = tier.FreeList;
    tier.FreeList = alloc;
}

usize TierAllocator::Description::GetTierIndex(const usize size) const
{
    return TKit::getTierIndex(size, MinAllocation, Granularity, Tiers.GetSize() - 1);
}
usize TierAllocator::getTierIndex(const usize size) const
{
    return TKit::getTierIndex(size, m_MinAllocation, m_Granularity, m_Tiers.GetSize() - 1);
}
} // namespace TKit
