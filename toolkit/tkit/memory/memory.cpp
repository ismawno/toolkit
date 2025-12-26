#include "tkit/core/pch.hpp"
#include "tkit/memory/memory.hpp"
#include "tkit/profiling/macros.hpp"
#include "tkit/preprocessor/utils.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/utils/bit.hpp"
#include "tkit/utils/limits.hpp"
#include "tkit/math/math.hpp"
#include "tkit/multiprocessing/topology.hpp"
#include <atomic>

#ifdef TKIT_ENABLE_GLOBAL_ALLOCATOR

#    define POWER_OF_TWO(x) ((x) > 0 && (((x) & ((x) - 1)) == 0))

#    ifndef TKIT_GLOBAL_ALLOCATOR_MAX_TIERS
#        define TKIT_GLOBAL_ALLOCATOR_MAX_TIERS 64
#    endif

#    define MEGAS(p_Megas) (p_Megas * 1024 * 1024)
#    define BYTES(p_Bytes) (p_Bytes / (1024 * 1024))

#    ifndef TKIT_GLOBAL_ALLOCATOR_ALIGNMENT
#        define TKIT_GLOBAL_ALLOCATOR_ALIGNMENT 16
#    endif
#    ifndef TKIT_GLOBAL_ALLOCATOR_MAX_ALLOCATION
#        define TKIT_GLOBAL_ALLOCATOR_MAX_ALLOCATION MEGAS(8)
#    endif
#    ifndef TKIT_GLOBAL_ALLOCATOR_GRANULARITY
#        define TKIT_GLOBAL_ALLOCATOR_GRANULARITY 4
#    endif
#    ifndef TKIT_GLOBAL_ALLOCATOR_TIER_SLOT_DECAY
#        define TKIT_GLOBAL_ALLOCATOR_TIER_SLOT_DECAY 0.9f
#    endif

#    if !POWER_OF_TWO(TKIT_GLOBAL_ALLOCATOR_MAX_ALLOCATION) || !POWER_OF_TWO(TKIT_GLOBAL_ALLOCATOR_GRANULARITY) ||     \
        !POWER_OF_TWO(TKIT_GLOBAL_ALLOCATOR_ALIGNMENT)
#        error                                                                                                         \
            "[TOOLKIT][ALLOCATOR] All integer arguments must be powers of two when creating a tier allocator description"
#    endif

#    if TKIT_GLOBAL_ALLOCATOR_ALIGNMENT > TKIT_CACHE_LINE_SIZE
#        error "[TOOLKIT][ALLOCATOR] Maximum alignment cannot be greater than the cache line size"
#    endif

#    if TKIT_GLOBAL_ALLOCATOR_GRANULARITY < 2
#        error "[TOOLKIT][ALLOCATOR] Granularity cannot be smaller than 2"
#    endif

#endif

namespace TKit::Memory
{
static void *vanillaAllocate(const size_t p_Size)
{
    return std::malloc(p_Size);
}

static void *vanillaAllocateAligned(const size_t p_Size, size_t p_Alignment)
{
    void *ptr = nullptr;
    p_Alignment = (p_Alignment + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
#ifdef TKIT_OS_WINDOWS
    ptr = _aligned_malloc(p_Size, p_Alignment);
#else
    int result = posix_memalign(&ptr, p_Alignment, p_Size);
    TKIT_UNUSED(result); // Sould do something with this at some point
#endif
    return ptr;
}

static void vanillaDeallocate(void *p_Ptr)
{
    std::free(p_Ptr);
}
static void vanillaDeallocateAligned(void *p_Ptr)
{
#ifdef TKIT_OS_WINDOWS
    _aligned_free(p_Ptr);
#else
    std::free(p_Ptr);
#endif
}

#ifdef TKIT_ENABLE_GLOBAL_ALLOCATOR
static_assert(TKIT_GLOBAL_ALLOCATOR_TIER_SLOT_DECAY > 0.f && TKIT_GLOBAL_ALLOCATOR_TIER_SLOT_DECAY <= 1.f,
              "[TOOLKIT][ALLOCATOR] Tier slot decay must be between 0 and 1.");

struct AllocationHeader
{
    u32 Magic;
    u16 ThreadIndex;
    u16 TierIndex;
};

struct Allocation
{
    Allocation *Next;
};

static constexpr usize s_Alignment = TKIT_GLOBAL_ALLOCATOR_ALIGNMENT;
static constexpr usize s_MaxAllocation = TKIT_GLOBAL_ALLOCATOR_MAX_ALLOCATION;

static constexpr usize s_HeaderSize =
    Math::Max(static_cast<usize>(sizeof(AllocationHeader)), static_cast<usize>(TKIT_GLOBAL_ALLOCATOR_ALIGNMENT));

static constexpr usize s_MinAllocation = TKit::Bit::NextPowerOfTwo(
    TKIT_GLOBAL_ALLOCATOR_ALIGNMENT + Math::Max(static_cast<usize>(sizeof(Allocation)), s_HeaderSize));

static constexpr usize s_Granularity = TKIT_GLOBAL_ALLOCATOR_GRANULARITY;
static constexpr f32 s_TierSlotDecay = TKIT_GLOBAL_ALLOCATOR_TIER_SLOT_DECAY;

static constexpr usize s_AllocMagic = 0xC0FFEE00u;
static constexpr usize s_FreeMagic = 0xDEADBEEFu;

static std::atomic_flag s_AllocatorReady = ATOMIC_FLAG_INIT;

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_MSVC_WARNING_IGNORE(4324)

struct Stack
{
    struct Node
    {
        void *Value;
        Node *Next;
    };

    Node *CreateNode(void *p_Value)
    {
        Node *node = t_FreeList;
        if (!node)
            node = m_FreeHead.exchange(nullptr, std::memory_order_acquire);

        if (node)
        {
            t_FreeList = node->Next;
            node->Value = p_Value;
            return node;
        }
        node = static_cast<Node *>(vanillaAllocate(sizeof(Node)));
        node->Value = p_Value;
        node->Next = nullptr;
        return node;
    }

    void Push(void *p_Value)
    {
        Node *oldHead = m_Head.load(std::memory_order_relaxed);
        Node *head = CreateNode(p_Value);
        do
        {
            head->Next = oldHead;
        } while (!m_Head.compare_exchange_weak(oldHead, head, std::memory_order_release, std::memory_order_relaxed));
    }

    Node *Acquire()
    {
        return m_Head.exchange(nullptr, std::memory_order_acquire);
    }

    void Reclaim(Node *p_Head, Node *p_Tail = nullptr)
    {
        Node *freeList = m_FreeHead.exchange(nullptr, std::memory_order_relaxed);
        if (!p_Tail)
        {
            p_Tail = p_Head;
            while (p_Tail->Next)
                p_Tail = p_Tail->Next;
        }

        p_Tail->Next = freeList;
        m_FreeHead.store(p_Head, std::memory_order_release);
    }

  private:
    alignas(TKIT_CACHE_LINE_SIZE) std::atomic<Node *> m_Head{nullptr};
    alignas(TKIT_CACHE_LINE_SIZE) std::atomic<Node *> m_FreeHead{nullptr};

    static inline thread_local Node *t_FreeList{};
};
TKIT_COMPILER_WARNING_IGNORE_POP()

struct Tier
{
    Allocation *FreeList;
};

struct TierInfo
{
    usize Size;
    usize AllocationSize;
    usize Slots;
};

struct AllocatorDescription
{
    StaticArray<TierInfo, TKIT_GLOBAL_ALLOCATOR_MAX_TIERS> Tiers;
    usize BufferSize;
    usize MaxAllocation;
    usize MinAllocation;
    usize Granularity;
    f32 TierSlotDecay;
};

static AllocatorDescription createDescription()
{
    AllocatorDescription desc{};
    desc.MaxAllocation = s_MaxAllocation;
    desc.MinAllocation = s_MinAllocation;
    desc.Granularity = s_Granularity;
    desc.TierSlotDecay = s_TierSlotDecay;

    TKIT_LOG_DEBUG("[TOOLKIT][ALLOCATOR] Maximum allocation is: {} mb", BYTES(desc.MaxAllocation));
    TKIT_LOG_DEBUG("[TOOLKIT][ALLOCATOR] Minimum allocation is: {} b", desc.MinAllocation);
    TKIT_LOG_DEBUG("[TOOLKIT][ALLOCATOR] Granularity is: {}", desc.Granularity);
    TKIT_LOG_DEBUG("[TOOLKIT][ALLOCATOR] Tier slot decay is: {}", desc.TierSlotDecay);

    usize size = s_MaxAllocation;
    usize prevAlloc = s_MaxAllocation;
    usize prevSlots = 1;

    const auto nextAlloc = [](const usize s_CurrentAlloc) {
        const usize increment = Bit::NextPowerOfTwo(s_CurrentAlloc) / s_Granularity;
        return s_CurrentAlloc - increment;
    };

    usize currentAlloc = nextAlloc(prevAlloc);
    usize currentSlots = 0;

    desc.Tiers.Append(TierInfo{.Size = s_MaxAllocation, .AllocationSize = s_MaxAllocation, .Slots = 1});

    const auto enoughSlots = [&currentSlots, &prevSlots]() {
        return static_cast<usize>(s_TierSlotDecay * currentSlots) >= prevSlots;
    };

    for (;;)
    {
        const usize psize = size;
        const usize alignment = Bit::PrevPowerOfTwo(currentAlloc);
        while (!enoughSlots() || (currentAlloc != s_MinAllocation && size % alignment != 0))
        {
            ++currentSlots;
            size += currentAlloc;
        }
        TierInfo tier{};
        tier.AllocationSize = currentAlloc;
        tier.Size = size - psize;
        TKIT_ASSERT(tier.Size % tier.AllocationSize == 0,
                    "[TOOLKIT][ALLOCATOR] Tier with size {} is not a perfect fit for the allocation size {}", tier.Size,
                    tier.AllocationSize);

        TKIT_ASSERT(tier.AllocationSize >= s_HeaderSize + sizeof(Allocation),
                    "[TOOLKIT][ALLOCATOR] Tier with size {} has an allocation size that is too small: {}. It should be "
                    "greater than {} + {} = {}",
                    tier.Size, tier.AllocationSize, s_HeaderSize, sizeof(Allocation),
                    s_HeaderSize + sizeof(Allocation));

        tier.Slots = tier.Size / tier.AllocationSize;
        TKIT_LOG_DEBUG(
            "[TOOLKIT][ALLOCATOR] Created tier {} of size {} mb ({} b) with an allocation size of {} mb ({} b) and "
            "with {} slots",
            desc.Tiers.GetSize(), BYTES(tier.Size), tier.Size, BYTES(tier.AllocationSize), tier.AllocationSize,
            tier.Slots);

        desc.Tiers.Append(tier);

        if (currentAlloc == s_MinAllocation)
            break;

        prevAlloc = currentAlloc;
        prevSlots = currentSlots;

        currentAlloc = nextAlloc(prevAlloc);
        currentSlots = 0;
    }

    desc.BufferSize = size;
    TKIT_LOG_DEBUG("[TOOLKIT][ALLOCATOR] Created {} tiers", desc.Tiers.GetSize());
    return desc;
}

struct alignas(TKIT_CACHE_LINE_SIZE) Pool
{
    StaticArray<Tier, TKIT_GLOBAL_ALLOCATOR_MAX_TIERS> Tiers{};
    Stack MailBox{};
};

static usize bitIndex(const usize p_Value)
{
    return static_cast<usize>(std::countr_zero(p_Value));
}

static AllocationHeader *getHeader(void *p_Ptr)
{
    return reinterpret_cast<AllocationHeader *>(static_cast<std::byte *>(p_Ptr) - s_HeaderSize);
}
static void *getUserMemory(AllocationHeader *p_Header)
{
    return reinterpret_cast<std::byte *>(p_Header) + s_HeaderSize;
}

struct Allocator
{
    Allocator()
    {
        const AllocatorDescription desc = createDescription();
        constexpr usize padding = (TKIT_MAX_THREADS - 1) * TKIT_CACHE_LINE_SIZE;
        const usize size = desc.BufferSize * TKIT_MAX_THREADS + padding;

        TKIT_LOG_DEBUG("[TOOLKIT][ALLOCATOR] Total memory per thread: {} mb", BYTES(desc.BufferSize));
        TKIT_LOG_DEBUG("[TOOLKIT][ALLOCATOR] Total cache line padding: {} b", padding);
        TKIT_LOG_DEBUG("[TOOLKIT][ALLOCATOR] Total size: {} mb", BYTES(size));

        Buffer = static_cast<std::byte *>(vanillaAllocateAligned(size, s_Alignment));
        TKIT_ASSERT(Buffer,
                    "[TOOLKIT][ALLOCATOR] Failed to allocate final allocator buffer of {} bytes aligned to {}. "
                    "Consider choosing another parameter combination",
                    desc.BufferSize, s_Alignment);

        std::memset(Buffer, 0, size);
        BufferSize = size;

#    ifdef TKIT_ENABLE_ASSERTS
        SetupMemoryLayout(desc, s_Alignment);
#    else
        SetupMemoryLayout(desc);
#    endif
    }

    bool Belongs(void *p_Ptr)
    {
        return p_Ptr >= Buffer && p_Ptr < Buffer + BufferSize;
    }

#    ifdef TKIT_ENABLE_ASSERTS
    void SetupMemoryLayout(const AllocatorDescription &p_Description, const usize p_MaxAlignment)
#    else
    void SetupMemoryLayout(const AllocatorDescription &p_Description)
#    endif
    {
        for (usize i = 0; i < TKIT_MAX_THREADS; ++i)
        {
            std::byte *baseBuffer = Buffer + i * (p_Description.BufferSize + TKIT_CACHE_LINE_SIZE);
            usize size = 0;
            for (usize j = 0; j < p_Description.Tiers.GetSize(); ++j)
            {
                const TierInfo &tinfo = p_Description.Tiers[j];

                Tier tier{};
                std::byte *buffer = baseBuffer + size;
                tier.FreeList = static_cast<Allocation *>(getUserMemory(reinterpret_cast<AllocationHeader *>(buffer)));
                const usize count = tinfo.Size / tinfo.AllocationSize;

                TKIT_ASSERT(
                    Memory::IsAligned(buffer, Math::Min(p_MaxAlignment, Bit::PrevPowerOfTwo(tinfo.AllocationSize))),
                    "[TOOLKIT][ALLOCATOR] Tier with size {} and buffer {} failed alignment check: it is not aligned "
                    "to either the maximum alignment or its previous power of 2",
                    tinfo.Size, static_cast<void *>(buffer));

                Allocation *next = nullptr;
                for (usize k = count - 1; k < count; --k)
                {
                    AllocationHeader *header = reinterpret_cast<AllocationHeader *>(buffer + k * tinfo.AllocationSize);
                    header->Magic = s_FreeMagic;
                    header->ThreadIndex = i;
                    header->TierIndex = j;

                    Allocation *alloc = static_cast<Allocation *>(getUserMemory(header));
                    alloc->Next = next;
                    next = alloc;
                }
                Pools[i].Tiers.Append(tier);
                size += tinfo.Size;
            }
        }
    }

    usize GetTierIndex(const usize p_ThreadIndex, const usize p_Size)
    {
        const usize lastIndex = Pools[p_ThreadIndex].Tiers.GetSize() - 1;
        if (p_Size <= s_MinAllocation)
            return lastIndex;

        const usize np2 = Bit::NextPowerOfTwo(p_Size);

        const usize grIndex = bitIndex(s_Granularity);
        const usize incIndex = bitIndex(np2 >> grIndex);
        const usize reference = np2 - p_Size;

        // Signed code for a bit more correctness, but as final result is guaranteed to not exceed uint max, it is not
        // strictly needed constexpr auto cast = [](const usize p_Value) { return static_cast<ssize>(p_Value); };
        //
        // const ssize offset = cast(bitIndex(s_MinAllocation)) - cast(bitIndex(s_Granularity));
        //
        // const ssize idx = cast(p_LastIndex) + cast(factor) * (offset - cast(incIndex)) + cast(reference / increment);
        // return static_cast<usize>(idx);
        const usize offset = bitIndex(s_MinAllocation) - grIndex;

        return lastIndex + ((offset - incIndex) << (grIndex - 1)) + (reference >> incIndex);
    }

    void *Allocate(const usize p_ThreadIndex, const usize p_Size)
    {
        const usize index = GetTierIndex(p_ThreadIndex, p_Size + s_HeaderSize);
        Tier &tier = Pools[p_ThreadIndex].Tiers[index];
        if (!tier.FreeList)
            return nullptr;

        AllocationHeader *header = reinterpret_cast<AllocationHeader *>(tier.FreeList);
        // TKIT_ASSERT(header->Magic == s_FreeMagic,
        //             "[TOOLKIT][ALLOCATOR] Magic number check failed. Expected: {}, found: {}", s_FreeMagic,
        //             header->Magic);
        tier.FreeList = tier.FreeList->Next;
        header->Magic = s_AllocMagic;
        header->ThreadIndex = p_ThreadIndex;
        header->TierIndex = index;
        return getUserMemory(header);
    }

    void DrainMailBox(const usize p_ThreadIndex)
    {
        Stack::Node *tail = Pools[p_ThreadIndex].MailBox.Acquire();
        if (!tail)
            return;

        Stack::Node *head = tail;
        while (tail->Next)
        {
            void *ptr = tail->Value;
            AllocationHeader *header = getHeader(ptr);
            Deallocate(p_ThreadIndex, ptr, header);
            tail = tail->Next;
        }
        void *ptr = tail->Value;
        AllocationHeader *header = getHeader(ptr);
        Deallocate(p_ThreadIndex, ptr, header);

        Pools[p_ThreadIndex].MailBox.Reclaim(head, tail);
    }

    void Deallocate(const usize p_ThreadIndex, void *p_Ptr, AllocationHeader *p_Header)
    {
        const usize index = p_Header->TierIndex;
        Tier &tier = Pools[p_ThreadIndex].Tiers[index];

        Allocation *alloc = static_cast<Allocation *>(p_Ptr);
        alloc->Next = tier.FreeList;
        tier.FreeList = alloc;
        p_Header->Magic = s_FreeMagic;
    }

    bool Deallocate(const usize p_ThreadIndex, void *p_Ptr)
    {
        AllocationHeader *header = getHeader(p_Ptr);
        if (header->Magic != s_AllocMagic)
            return false;
        // TKIT_ASSERT(header->Magic == s_AllocMagic,
        //             "[TOOLKIT][ALLOCATOR] Magic number check failed. Expected: {}, found: {}", s_AllocMagic,
        //             header->Magic);

        const usize tindex = header->ThreadIndex;
        if (tindex != p_ThreadIndex)
            Pools[tindex].MailBox.Push(p_Ptr);
        else
            Deallocate(p_ThreadIndex, p_Ptr, header);

        DrainMailBox(p_ThreadIndex);
        return true;
    }

    TKit::Array<Pool, TKIT_MAX_THREADS> Pools{};
    std::byte *Buffer;
    usize BufferSize;
    usize MinAllocation;
    usize Granularity;
};

static Allocator s_Allocator;
#endif

void *Allocate(const size_t p_Size)
{
#ifdef TKIT_ENABLE_GLOBAL_ALLOCATOR
    const usize tindex = Topology::GetThreadIndex();
    void *ptr;
    if (tindex == TKit::Limits<usize>::Max() || !s_AllocatorReady.test(std::memory_order_relaxed))
        ptr = vanillaAllocate(p_Size);
    else
        ptr = s_Allocator.Allocate(tindex, static_cast<usize>(p_Size));
#else
    void *ptr = vanillaAllocate(p_Size);
#endif
    TKIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}

void Deallocate(void *p_Ptr)
{
    TKIT_PROFILE_MARK_DEALLOCATION(p_Ptr);
#ifdef TKIT_ENABLE_GLOBAL_ALLOCATOR
    const usize tindex = Topology::GetThreadIndex();
    if (tindex == TKit::Limits<usize>::Max() || !s_AllocatorReady.test(std::memory_order_relaxed) ||
        !s_Allocator.Deallocate(tindex, p_Ptr))
        vanillaDeallocate(p_Ptr);
#else
    vanillaDeallocate(p_Ptr);
#endif
}

void *AllocateAligned(const size_t p_Size, const size_t p_Alignment)
{
#ifdef TKIT_ENABLE_GLOBAL_ALLOCATOR
    const usize tindex = Topology::GetThreadIndex();
    void *ptr;
    if (p_Alignment > s_Alignment || tindex == TKit::Limits<usize>::Max() ||
        !s_AllocatorReady.test(std::memory_order_relaxed))
        ptr = vanillaAllocateAligned(p_Size, p_Alignment);
    else
        ptr = s_Allocator.Allocate(tindex, static_cast<usize>(p_Size));
#else
    void *ptr = vanillaAllocateAligned(p_Size, p_Alignment);
#endif
    TKIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}

void DeallocateAligned(void *p_Ptr)
{
    TKIT_PROFILE_MARK_DEALLOCATION(p_Ptr);
#ifdef TKIT_ENABLE_GLOBAL_ALLOCATOR
    const usize tindex = Topology::GetThreadIndex();
    if (tindex == TKit::Limits<usize>::Max() || !s_AllocatorReady.test(std::memory_order_relaxed) ||
        !s_Allocator.Belongs(p_Ptr))
        vanillaDeallocateAligned(p_Ptr);
    else
        s_Allocator.Deallocate(tindex, p_Ptr);
#else
    vanillaDeallocateAligned(p_Ptr);
#endif
}

bool IsAligned(const void *p_Ptr, const size_t p_Alignment)
{
    const uptr addr = reinterpret_cast<uptr>(p_Ptr);
    return (addr & (p_Alignment - 1)) == 0;
}

bool IsAligned(const size_t p_Address, const size_t p_Alignment)
{
    return (p_Address & (p_Alignment - 1)) == 0;
}

void *ForwardCopy(void *p_Dst, const void *p_Src, size_t p_Size)
{
    return std::memcpy(p_Dst, p_Src, p_Size);
}
void *BackwardCopy(void *p_Dst, const void *p_Src, size_t p_Size)
{
    return std::memmove(p_Dst, p_Src, p_Size);
}

} // namespace TKit::Memory

#ifdef TKIT_ENABLE_GLOBAL_ALLOCATOR
namespace TKit::Detail
{
void AllocatorReady()
{
    Memory::s_AllocatorReady.test_and_set(std::memory_order_relaxed);
}
} // namespace TKit::Detail
#endif

#ifndef TKIT_DISABLE_MEMORY_OVERRIDES
void *operator new(const size_t p_Size)
{
    return TKit::Memory::Allocate(p_Size);
}
void *operator new[](const size_t p_Size)
{
    return TKit::Memory::Allocate(p_Size);
}
void *operator new(const size_t p_Size, const std::align_val_t p_Alignment)
{
    return TKit::Memory::AllocateAligned(p_Size, static_cast<size_t>(p_Alignment));
}
void *operator new[](const size_t p_Size, const std::align_val_t p_Alignment)
{
    return TKit::Memory::AllocateAligned(p_Size, static_cast<size_t>(p_Alignment));
}
void *operator new(const size_t p_Size, const std::nothrow_t &) noexcept
{
    return TKit::Memory::Allocate(p_Size);
}
void *operator new[](const size_t p_Size, const std::nothrow_t &) noexcept
{
    return TKit::Memory::Allocate(p_Size);
}

void operator delete(void *p_Ptr) noexcept
{
    TKit::Memory::Deallocate(p_Ptr);
}
void operator delete[](void *p_Ptr) noexcept
{
    TKit::Memory::Deallocate(p_Ptr);
}
void operator delete(void *p_Ptr, std::align_val_t) noexcept
{
    TKit::Memory::DeallocateAligned(p_Ptr);
}
void operator delete[](void *p_Ptr, std::align_val_t) noexcept
{
    TKit::Memory::DeallocateAligned(p_Ptr);
}
void operator delete(void *p_Ptr, const std::nothrow_t &) noexcept
{
    TKit::Memory::Deallocate(p_Ptr);
}
void operator delete[](void *p_Ptr, const std::nothrow_t &) noexcept
{
    TKit::Memory::Deallocate(p_Ptr);
}

void operator delete(void *p_Ptr, size_t) noexcept
{
    TKit::Memory::Deallocate(p_Ptr);
}
void operator delete[](void *p_Ptr, size_t) noexcept
{
    TKit::Memory::Deallocate(p_Ptr);
}
void operator delete(void *p_Ptr, size_t, std::align_val_t) noexcept
{
    TKit::Memory::DeallocateAligned(p_Ptr);
}
void operator delete[](void *p_Ptr, size_t, std::align_val_t) noexcept
{
    TKit::Memory::DeallocateAligned(p_Ptr);
}
void operator delete(void *p_Ptr, size_t, const std::nothrow_t &) noexcept
{
    TKit::Memory::Deallocate(p_Ptr);
}
void operator delete[](void *p_Ptr, size_t, const std::nothrow_t &) noexcept
{
    TKit::Memory::Deallocate(p_Ptr);
}

#endif
