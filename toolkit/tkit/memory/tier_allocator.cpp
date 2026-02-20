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

static void createDefaultSlotRequests(ArenaArray<usize> &slots, const f32 tierSlotDecay)
{
    const usize capacity = slots.GetCapacity();
    TKIT_ASSERT(capacity != 0, "[TOOLKIT][TIER-ALLOC] Maximum tiers must not be zero");

    slots.Append(1);
    for (usize i = 1; i < capacity; ++i)
    {
        const usize prev = slots[i - 1];
        slots.Append(static_cast<usize>(prev / tierSlotDecay) + 1);
    }
}

TierDescriptions::TierDescriptions(const TierSpecs &specs)
    : m_Tiers(specs.Allocator, specs.MaxTiers), m_MinSlots(specs.Allocator, specs.MaxTiers),
      m_Granularity(specs.Granularity),
      m_MinAllocation(specs.MinAllocation ? specs.MinAllocation : (specs.Granularity * sizeof(void *) / 2)),
      m_MaxAllocation(specs.MaxAllocation)
{
    TKIT_ASSERT(IsPowerOfTwo(m_MaxAllocation) && IsPowerOfTwo(m_MinAllocation) && IsPowerOfTwo(m_Granularity),
                "[TOOLKIT][TIER-ALLOC] All integer arguments must be powers of two when creating a tier allocator "
                "description, but the values where {}, {} and {}",
                m_MaxAllocation, m_MinAllocation, m_Granularity);
    TKIT_ASSERT(m_Granularity <= m_MinAllocation,
                "[TOOLKIT][TIER-ALLOC] Granularity ({}) must be less or equal than the minimum allocation ({})",
                m_Granularity, m_MinAllocation);
    TKIT_ASSERT(m_Granularity >= 2, "[TOOLKIT][TIER-ALLOC] Granularity cannot be smaller than 2, but its value was {}",
                m_Granularity);
    TKIT_ASSERT(specs.TierSlotDecay > 0.f && specs.TierSlotDecay <= 1.f,
                "[TOOLKIT][TIER-ALLOC] Tier slot decay must be between 0.0 and 1.0, but its value was {}",
                specs.TierSlotDecay);
    TKIT_ASSERT(
        2 * m_MinAllocation >= sizeof(void *) * m_Granularity,
        "[TOOLKIT][TIER-ALLOC] The minimum allocation must at least be granularity * sizeof(void *) / 2 = {}, but "
        "passed value was {}",
        m_Granularity * sizeof(void *) / 2, m_MinAllocation);

    createDefaultSlotRequests(m_MinSlots, specs.TierSlotDecay);
    buildTierLayout();
}

usize TierDescriptions::GetTierIndex(const usize size) const
{
    return TKit::getTierIndex(size, m_MinAllocation, m_Granularity, m_Tiers.GetSize() - 1);
}

void TierDescriptions::buildTierLayout()
{
    m_Tiers.Clear();

    const auto nextAlloc = [this](const usize currentAlloc) {
        const usize increment = NextPowerOfTwo(currentAlloc) / m_Granularity;
        TKIT_ASSERT(increment % sizeof(void *) == 0,
                    "[TOOLKIT][TIER-ALLOC] Increments in memory between tiers must all be divisible by sizeof(void *) "
                    "= {}, but found an increment of {}. To avoid this error, ensure that minAllocation >= granularity "
                    "* sizeof(void *)",
                    sizeof(void *), increment);
        return currentAlloc - increment;
    };

    m_BufferSize = m_MaxAllocation;
    usize currentAlloc = nextAlloc(m_MaxAllocation);

    m_Tiers.Append(TierInfo{.Size = m_MaxAllocation, .AllocationSize = m_MaxAllocation, .Slots = 1});
    for (;;)
    {
        const usize alignment = PrevPowerOfTwo(currentAlloc);

        usize slots = m_MinSlots[m_Tiers.GetSize()];
        usize size = slots * currentAlloc;
        while (size % alignment != 0)
        {
            ++slots;
            size += currentAlloc;
        }

        TierInfo tier{};
        tier.AllocationSize = currentAlloc;
        tier.Size = size;
        m_BufferSize += size;
        TKIT_ASSERT(tier.Size % tier.AllocationSize == 0,
                    "[TOOLKIT][TIER-ALLOC] Tier with size {} is not a perfect fit for the allocation size {}",
                    tier.Size, tier.AllocationSize);

        tier.Slots = tier.Size / tier.AllocationSize;
        m_Tiers.Append(tier);

        if (currentAlloc == m_MinAllocation)
            break;
        currentAlloc = nextAlloc(currentAlloc);
    }
#ifdef TKIT_ENABLE_ASSERTS
    const auto slowIndex = [this](const usize size) {
        for (usize i = m_Tiers.GetSize() - 1; i < m_Tiers.GetSize(); --i)
            if (m_Tiers[i].AllocationSize >= size)
                return i;
        return m_Tiers.GetSize();
    };
    for (usize mem = m_MinAllocation; mem <= m_MaxAllocation; ++mem)
    {
        const usize index = GetTierIndex(mem);
        TKIT_ASSERT(m_Tiers[index].AllocationSize >= mem,
                    "[TOOLKIT][TIER-ALLOC] Allocator is malformed. Found a size of {:L} being assigned a tier index of "
                    "{} with a smaller allocation size of {:L}",
                    mem, index, m_Tiers[index].AllocationSize);
        TKIT_ASSERT(index == m_Tiers.GetSize() - 1 || m_Tiers[index + 1].AllocationSize < mem,
                    "[TOOLKIT][TIER-ALLOC] Allocator is malformed. Found a size of {:L} being assigned a tier index of "
                    "{} with an allocation size of {:L}, but tier index {} has a big enough allocation size of {:L}",
                    mem, index, m_Tiers[index].AllocationSize, index + 1, m_Tiers[index + 1].AllocationSize);
        const usize sindex = slowIndex(mem);
        TKIT_ASSERT(sindex == index,
                    "[TOOLKIT][TIER-ALLOC] Allocator is malformed. Brute forced tier index discovery of {} for a size "
                    "of {:L} bytes, while the fast approach computed {}",
                    sindex, mem, index);
    }
#endif
}

TierAllocator::TierAllocator(const TierDescriptions &tiers, const usize maxAlignment)
    : m_Tiers(tiers.GetTiers().GetAllocator(), tiers.GetTiers().GetCapacity()), m_BufferSize(tiers.GetBufferSize()),
      m_MinAllocation(tiers.GetMinAllocation()), m_Granularity(tiers.GetGranularity())
{
#ifdef TKIT_ENABLE_ASSERTS
    m_MaxAllocation = tiers.GetMaxAllocation();
#endif
    TKIT_ASSERT(IsPowerOfTwo(maxAlignment),
                "[TOOLKIT][TIER-ALLOC] Maximum alignment must be a power of 2, but {} is not", maxAlignment);
    m_Buffer = static_cast<std::byte *>(AllocateAligned(m_BufferSize, maxAlignment));
#ifdef TKIT_ENABLE_ASSERTS
    setupMemoryLayout(tiers, maxAlignment);
#else
    setupMemoryLayout(tiers);
#endif
}

TierAllocator::TierAllocator(const TierSpecs &specs, const usize maxAlignment)
    : TierAllocator(TierDescriptions{specs}, maxAlignment)
{
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
void TierAllocator::setupMemoryLayout(const TierDescriptions &tiers, const usize maxAlignment)
#else
void TierAllocator::setupMemoryLayout(const TierDescriptions &tiers)
#endif
{
    usize size = 0;
    for (const TierInfo &tinfo : tiers.GetTiers())
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

usize TierAllocator::getTierIndex(const usize size) const
{
    return TKit::getTierIndex(size, m_MinAllocation, m_Granularity, m_Tiers.GetSize() - 1);
}
} // namespace TKit
