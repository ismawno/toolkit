#include "tkit/core/pch.hpp"
#include "tkit/memory/tier_allocator.hpp"
#include "tkit/utils/bit.hpp"
#include "tkit/utils/debug.hpp"
#include "tkit/profiling/macros.hpp"
#include "tkit/math/math.hpp"

namespace TKit
{
static usize logp2(const usz value)
{
    return usize(std::countr_zero(value));
}
// the idea of this allocator is to, from a given size, derive its tier with very simple operations (avoid iterating all
// tiers). turns out that it is possible. we can define the allocation size of a given tier as:
//
// - T_{i+1} = T_{i} - NextPowerOfTwo(T_{i}) / G;
//
// where
//
// - T_{i} is the size of tier at index i (i goes from 0 to n-1)
// - G is the granularity, which is a provided power of 2
//
// such that T{i+1} < T{i}. T_{0} is provided by the user as the maximum allocation size.
// each tier may have any number of slots available, such that you can fit N allocations
// of course, an allocation of size S will belong to the tier such that T_{i} > S and T_{i+1} < S
//
// the total size of a tier T_{i} is straightforward: ST_{i} = N * T_{i}
//
// once we define this relationship, we can control the granularity of each tier and how much each tier differs from its
// neighbor. however, when allocating, we are provided a size, and we must identify (quickly) what tier it belongs to.
// luckily, because the relationship we have between tiers is well-defined, i was able to derive a little formula that,
// given a size, returns the tier index it belongs to
//
// I(S) = (n-1) - ( log2(Np2(S)) - log2(minAlloc) ) * granularity / 2 + granularity * (Np2(S) - S) / Np2(S)
//
// where
//
// - I is the tier index
// - S is the requested allocation size
// - Np2() returns the next power of 2
// - minAlloc is the minimum tier allocation size this allocator supports, provided by user
// - granularity is how close in allocation size each tier is from its neighbors
static usize getTierIndex(const usz size, const usz minAllocation, const usize granularity, const usize lastIndex)
{
    if (size <= minAllocation)
        return lastIndex;

    const usz np2 = NextPowerOfTwo(size);
    const usize lognp2 = logp2(np2);

    return lastIndex - (lognp2 - logp2(minAllocation)) * (granularity >> 1) +
           ((granularity * usize(np2 - size)) >> lognp2);

    // this is the old rusty implementation i had where i hadnt formalized the expression well :(
    // const usz np2 = NextPowerOfTwo(size);
    //
    // const usize grIndex = bitIndex(granularity);
    // const usize incIndex = bitIndex(np2 >> grIndex);
    // const usize reference = usize(np2 - size);
    //
    // // Signed code for a bit more correctness, but as final result is guaranteed to not exceed uint max, it is not
    // // strictly needed constexpr auto cast = [](const usize value) { return scast<ssize>(value); };
    // //
    // // const ssize offset = cast(bitIndex(minAllocation)) - cast(bitIndex(granularity));
    // //
    // // const ssize idx = cast(lastIndex) + cast(factor) * (offset - cast(incIndex)) + cast(reference / increment);
    // // return usize(idx);
    // const usize offset = bitIndex(minAllocation) - grIndex;
    //
    // return lastIndex + ((offset - incIndex) << (grIndex - 1)) + (reference >> incIndex);
}

static void createDefaultSlotRequests(ArenaArray<usize> &slots, const f32 tierSlotDecay)
{
    const usize capacity = slots.GetCapacity();
    TKIT_ASSERT(capacity != 0, "[TOOLKIT][TIER-ALLOC] Maximum tiers must not be zero");

    slots.Append(1);
    for (usize i = 1; i < capacity; ++i)
    {
        const usize prev = slots[i - 1];
        slots.Append(usize(f32(prev) / tierSlotDecay) + 1);
    }
}

TierDescriptions::TierDescriptions(const TierSpecs &specs)
    : m_Tiers(specs.Allocator, specs.MaxTiers), m_MinSlots(specs.Allocator, specs.MaxTiers),
      m_MinAllocation(specs.MinAllocation ? specs.MinAllocation : (specs.Granularity * sizeof(void *) / 2)),
      m_MaxAllocation(specs.MaxAllocation), m_Granularity(specs.Granularity)
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

usize TierDescriptions::GetTierIndex(const usz size) const
{
    return TKit::getTierIndex(size, m_MinAllocation, m_Granularity, m_Tiers.GetSize() - 1);
}

void TierDescriptions::buildTierLayout()
{
    m_Tiers.Clear();

    const auto nextAlloc = [this](const usz currentAlloc) {
        const usize increment = usize(NextPowerOfTwo(currentAlloc) / m_Granularity);
        TKIT_ASSERT(increment % sizeof(void *) == 0,
                    "[TOOLKIT][TIER-ALLOC] Increments in memory between tiers must all be divisible by sizeof(void *) "
                    "= {}, but found an increment of {}. To avoid this error, ensure that minAllocation >= granularity "
                    "* sizeof(void *)",
                    sizeof(void *), increment);
        return currentAlloc - increment;
    };

    m_BufferSize = m_MaxAllocation;
    usz currentAlloc = nextAlloc(m_MaxAllocation);

    m_Tiers.Append(TierInfo{.Size = m_MaxAllocation, .AllocationSize = m_MaxAllocation, .Slots = 1});
    for (;;)
    {
        const usize alignment = usize(PrevPowerOfTwo(currentAlloc));

        usize slots = m_MinSlots[m_Tiers.GetSize()];
        usz size = slots * currentAlloc;
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

        tier.Slots = usize(tier.Size / tier.AllocationSize);
        m_Tiers.Append(tier);

        if (currentAlloc == m_MinAllocation)
            break;
        currentAlloc = nextAlloc(currentAlloc);
    }
#ifdef TKIT_ENABLE_ENSURE
    const auto slowIndex = [this](const usz size) {
        for (usize i = m_Tiers.GetSize() - 1; i < m_Tiers.GetSize(); --i)
            if (m_Tiers[i].AllocationSize >= size)
                return i;
        return m_Tiers.GetSize();
    };
    for (usz mem = m_MinAllocation; mem <= m_MaxAllocation; ++mem)
    {
        const usize index = GetTierIndex(mem);
        TKIT_ENSURE(m_Tiers[index].AllocationSize >= mem,
                    "[TOOLKIT][TIER-ALLOC] Allocator is malformed. Found a size of {:L} being assigned a tier index of "
                    "{} with a smaller allocation size of {:L}",
                    mem, index, m_Tiers[index].AllocationSize);
        TKIT_ENSURE(index == m_Tiers.GetSize() - 1 || m_Tiers[index + 1].AllocationSize < mem,
                    "[TOOLKIT][TIER-ALLOC] Allocator is malformed. Found a size of {:L} being assigned a tier index of "
                    "{} with an allocation size of {:L}, but tier index {} has a big enough allocation size of {:L}",
                    mem, index, m_Tiers[index].AllocationSize, index + 1, m_Tiers[index + 1].AllocationSize);
        const usize sindex = slowIndex(mem);
        TKIT_ENSURE(sindex == index,
                    "[TOOLKIT][TIER-ALLOC] Allocator is malformed. Brute forced tier index discovery of {} for a size "
                    "of {:L} bytes, while the fast approach computed {}",
                    sindex, mem, index);
    }
#endif
}

TierAllocator::TierAllocator(const TierDescriptions &tiers, const usize maxAlignment, const usize headerAllocsAlignment)
    : m_Tiers(tiers.GetTiers().GetAllocator(), tiers.GetTiers().GetCapacity()), m_BufferSize(tiers.GetBufferSize()),
      m_MinAllocation(tiers.GetMinAllocation()), m_Granularity(tiers.GetGranularity()),
      m_HeaderAllocationsAlignment(headerAllocsAlignment)
{
#ifdef TKIT_ENABLE_ENSURE
    m_MaxAllocation = tiers.GetMaxAllocation();
#endif
    TKIT_ASSERT(IsPowerOfTwo(maxAlignment),
                "[TOOLKIT][TIER-ALLOC] Maximum alignment must be a power of 2, but {} is not", maxAlignment);
    TKIT_ASSERT(maxAlignment >= alignof(std::max_align_t),
                "[TOOLKIT][TIER-ALLOC] Maximum alignment must be greater or equal than alignof(std::max_align_t)");
    TKIT_ASSERT(IsPowerOfTwo(headerAllocsAlignment),
                "[TOOLKIT][TIER-ALLOC] Header allocations alignment must be a power of 2, but {} is not",
                headerAllocsAlignment);
    TKIT_ASSERT(
        headerAllocsAlignment >= alignof(std::max_align_t),
        "[TOOLKIT][TIER-ALLOC] Header allocations alignment must be greater or equal than alignof(std::max_align_t)");
    m_Buffer = scast<std::byte *>(AllocateAligned(m_BufferSize, maxAlignment));
#ifdef TKIT_ENABLE_ENSURE
    setupMemoryLayout(tiers, maxAlignment);
#else
    setupMemoryLayout(tiers);
#endif
}

TierAllocator::TierAllocator(const TierSpecs &specs, const usize maxAlignment, const usize headerAllocsAlignment)
    : TierAllocator(TierDescriptions{specs}, maxAlignment, headerAllocsAlignment)
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

#ifdef TKIT_ENABLE_ENSURE
void TierAllocator::setupMemoryLayout(const TierDescriptions &tiers, const usize maxAlignment)
#else
void TierAllocator::setupMemoryLayout(const TierDescriptions &tiers)
#endif
{
    usz size = 0;
    for (const TierInfo &tinfo : tiers.GetTiers())
    {
        Tier tier{};
        tier.Buffer = m_Buffer + size;
        tier.FreeList = rcast<Allocation *>(tier.Buffer);
        const usize count = usize(tinfo.Size / tinfo.AllocationSize);

        TKIT_ENSURE(IsAligned(tier.Buffer, Math::Min(usz(maxAlignment), PrevPowerOfTwo(tinfo.AllocationSize))),
                    "[TOOLKIT][TIER-ALLOC] Tier with size {:L} and buffer {} failed alignment check: it is not aligned "
                    "to either the maximum alignment ({}) or its previous power of 2 ({})",
                    tinfo.Size, scast<void *>(tier.Buffer), maxAlignment, PrevPowerOfTwo(tinfo.AllocationSize));

        Allocation *next = nullptr;
        for (usize i = count - 1; i < count; --i)
        {
            Allocation *alloc = rcast<Allocation *>(tier.Buffer + i * tinfo.AllocationSize);
            TKIT_ASSERT(IsAligned(alloc, alignof(Allocation)),
                        "[TOOLKIT][TIER-ALLOC] Allocation landed in a memory region where its alignment of {} is not "
                        "respected. This happened when using an allocation size of {:L}",
                        alignof(Allocation), tinfo.AllocationSize);
            alloc->Next = next;
            next = alloc;
        }
#ifdef TKIT_ENABLE_ENSURE
        tier.Slots = tinfo.Slots;
        tier.Size = tinfo.Size;
        tier.AllocationSize = tinfo.AllocationSize;
#endif
        m_Tiers.Append(tier);
        size += tinfo.Size;
    }

    TKIT_POISON_MEMORY_REGION(m_Buffer, m_BufferSize);
}

void TierAllocator::deallocateBuffer()
{
    if (m_Buffer)
    {
#ifdef TKIT_ENABLE_ENSURE
        for (u32 i = 0; i < m_Tiers.GetSize(); ++i)
        {
            const Tier &tier = m_Tiers[i];
            TKIT_ENSURE(tier.Allocations >= tier.Deallocations,
                        "[TOOLKIT][TIER-ALLOC] Found tier index {} to have more deallocations ({}) than deallocations "
                        "({}), meaning a "
                        "double free likely happened and this allocator became corrupted",
                        i, tier.Allocations, tier.Deallocations);
            TKIT_ENSURE(
                tier.Allocations == tier.Deallocations,
                "[TOOLKIT][TIER-ALLOC] Found tier index {} to have {} allocations and {} deallocations, meaning "
                "{} active allocations remain when destroying this allocator",
                i, tier.Allocations, tier.Deallocations, tier.Allocations - tier.Deallocations);
        }
#endif
        DeallocateAligned(m_Buffer);
    }
}

void *TierAllocator::allocate(const usize tierIndex, const usz size)
{
    Tier &tier = m_Tiers[tierIndex];
    if (!tier.FreeList)
    {
        void *ptr = tierIndex != 0 ? allocate(tierIndex - 1, size) : nullptr;
#ifdef TKIT_ENABLE_ENSURE
        if (ptr)
        {
            const usize index = getTierIndex(size);
            if (index == tierIndex)
            {
                ++tier.Slots;
                ++tier.SlotsStolen;
                ++tier.Allocations;
            }
            TKIT_ENSURE((tier.Allocations - tier.Deallocations) <= tier.Slots,
                        "[TOOLKIT][TIER-ALLOC] Allocator is malformed. Tier of index {} (with allocation of size {:L}) "
                        "exceeded slots "
                        "(allocations - deallocations) = ({} - {}) = {} > slots = {}, but allocator did not attempt to "
                        "return nullptr",
                        tierIndex, size, tier.Allocations, tier.Deallocations, tier.Allocations - tier.Deallocations,
                        tier.Slots);
        }
#endif
        TKIT_LOG_WARNING_IF(ptr,
                            "[TOOLKIT][TIER-ALLOC] Allocator ran out of slots when trying to perform an allocation for "
                            "tier index {} and size {:L}. A slot was stolen from tier index {}",
                            tierIndex, size, tierIndex - 1);
        TKIT_LOG_ERROR_IF(tierIndex == 0 && !ptr,
                          "[TOOLKIT][TIER-ALLOC] Allocator ran out of memory when trying to perform an allocation for "
                          "tier index {} and size {:L}",
                          tierIndex, size);
        return ptr;
    }
#ifdef TKIT_ENABLE_ENSURE
    const usize index = getTierIndex(size);
    TKIT_ENSURE(index >= tierIndex,
                "[TOOLKIT][TIER-ALLOC] Trying to allocate {:L} bytes that map to the tier index {}, but are being "
                "allocated in tier index {} which has insufficient capacity for it",
                size, index, tierIndex);
    if (index != tierIndex)
    {
        --tier.Slots; // we are being robbed
        ++tier.SlotsRemoved;
    }
    else
    {
        TKIT_ENSURE((++tier.Allocations - tier.Deallocations) <= tier.Slots,
                    "[TOOLKIT][TIER-ALLOC] Allocator is malformed. Tier of index {} (with allocation of size {:L}) "
                    "exceeded slots "
                    "(allocations - deallocations) = ({} - {}) = {} > slots = {}, but allocator did not attempt to "
                    "return nullptr",
                    tierIndex, size, tier.Allocations, tier.Deallocations, tier.Allocations - tier.Deallocations,
                    tier.Slots);
    }
#endif

    Allocation *alloc = tier.FreeList;
    TKIT_UNPOISON_MEMORY_REGION(alloc, Math::Max(size, sizeof(Allocation)));
    tier.FreeList = alloc->Next;

    TKIT_PROFILE_MARK_POOL_ALLOCATION("tier-allocator", alloc, size);
#ifdef TKIT_ENABLE_ENSURE
    ++m_Allocations;
#endif
    return alloc;
}
void *TierAllocator::Allocate(const usz size)
{
    TKIT_ENSURE(size <= m_MaxAllocation,
                "[TOOLKIT][TIER-ALLOC] Allocation of size {:L} bytes exceeds max allocation size of {:L}", size,
                m_MaxAllocation);
    const usize index = getTierIndex(size);
    return allocate(index, size);
}

void TierAllocator::Deallocate(const void *ptr, const usz size)
{
    TKIT_ASSERT(ptr, "[TOOLKIT][TIER-ALLOC] Cannot deallocate a null pointer");
    TKIT_ASSERT(Belongs(ptr),
                "[TOOLKIT][TIER-ALLOC] Cannot deallocate a pointer that does not belong to the allocator");

    const usize index = getTierIndex(size);
    Tier &tier = m_Tiers[index];
    TKIT_ENSURE(tier.Allocations >= ++tier.Deallocations,
                "[TOOLKIT][TIER-ALLOC] Attempting to deallocate more times than the amount of active alocations there "
                "are for the tier index {} and size {:L}, with {} allocations and {} deallocations",
                index, size, tier.Allocations, tier.Deallocations);

    Allocation *alloc = scast<Allocation *>(ccast<void *>(ptr));
    TKIT_PROFILE_MARK_POOL_DEALLOCATION("tier-allocator", alloc);
    alloc->Next = tier.FreeList;
    TKIT_POISON_MEMORY_REGION(alloc, size);
    tier.FreeList = alloc;
#ifdef TKIT_ENABLE_ENSURE
    ++m_Deallocations;
#endif
}

void *TierAllocator::AllocateWithHeader(const usz size)
{
    const usz headerSize = GetHeaderSize();
    void *ptr = Allocate(size + headerSize);
    if (!ptr)
        return nullptr;

    usz *header = rcast<usz *>(ptr);
    *header = size + headerSize;
    TKIT_POISON_MEMORY_REGION(ptr, headerSize);
    return rcast<std::byte *>(ptr) + headerSize;
}

void TierAllocator::DeallocateWithHeader(const void *ptr)
{
    TKIT_ASSERT(ptr, "[TOOLKIT][TIER-ALLOC] Cannot deallocate a null pointer");
    const usz headerSize = GetHeaderSize();

    const std::byte *mem = rcast<const std::byte *>(ptr);
    const usz *header = rcast<const usz *>(mem - headerSize);
    TKIT_UNPOISON_MEMORY_REGION(header, headerSize);
    Deallocate(scast<const void *>(header), *header);
}

usize TierAllocator::getTierIndex(const usz size) const
{
    return TKit::getTierIndex(size, m_MinAllocation, m_Granularity, m_Tiers.GetSize() - 1);
}
} // namespace TKit
