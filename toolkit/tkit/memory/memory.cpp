#include "tkit/core/pch.hpp"
#include "tkit/memory/memory.hpp"
#include "tkit/profiling/macros.hpp"
#include "tkit/preprocessor/utils.hpp"
#include "tkit/utils/limits.hpp"
#include "tkit/container/fixed_array.hpp"
#include "tkit/multiprocessing/topology.hpp"
#ifdef TKIT_ENABLE_MIMALLOC
#    include <mimalloc-new-delete.h>
#    include <mimalloc-override.h>
#endif
#include <cstring>

namespace TKit::Memory
{
static thread_local FixedArray<ArenaAllocator *, MaxAllocatorPushDepth> s_Arenas{};
static thread_local FixedArray<StackAllocator *, MaxAllocatorPushDepth> s_Stacks{};
static thread_local FixedArray<TierAllocator *, MaxAllocatorPushDepth> s_Tiers{};

static thread_local usize s_ArenaSize = 0;
static thread_local usize s_StackSize = 0;
static thread_local usize s_TierSize = 0;

void PushArena(ArenaAllocator *alloc)
{
    s_Arenas[s_ArenaSize++] = alloc;
}
void PushStack(StackAllocator *alloc)
{
    s_Stacks[s_StackSize++] = alloc;
}
void PushTier(TierAllocator *alloc)
{
    s_Tiers[s_TierSize++] = alloc;
}

ArenaAllocator *GetArena()
{
    return s_ArenaSize == 0 ? nullptr : s_Arenas[s_ArenaSize - 1];
}
StackAllocator *GetStack()
{
    return s_StackSize == 0 ? nullptr : s_Stacks[s_StackSize - 1];
}
TierAllocator *GetTier()
{
    return s_TierSize == 0 ? nullptr : s_Tiers[s_TierSize - 1];
}

void PopArena()
{
    --s_ArenaSize;
}
void PopStack()
{
    --s_StackSize;
}
void PopTier()
{
    --s_TierSize;
}

void *Allocate(const size_t size)
{
    void *ptr = malloc(size);
    TKIT_PROFILE_MARK_ALLOCATION(ptr, size);
    return ptr;
}

void Deallocate(void *ptr)
{
    TKIT_PROFILE_MARK_DEALLOCATION(ptr);
    free(ptr);
}

void *AllocateAligned(const size_t size, size_t alignment)
{
    void *ptr = nullptr;
    alignment = (alignment + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
#ifdef TKIT_OS_WINDOWS
    ptr = _aligned_malloc(size, alignment);
#else
    int result = posix_memalign(&ptr, alignment, size);
    TKIT_UNUSED(result); // Sould do something with this at some point
#endif
    TKIT_PROFILE_MARK_ALLOCATION(ptr, size);
    return ptr;
}

void DeallocateAligned(void *ptr)
{
    TKIT_PROFILE_MARK_DEALLOCATION(ptr);
#ifdef TKIT_OS_WINDOWS
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

void *ForwardCopy(void *dst, const void *src, size_t size)
{
    return std::memcpy(dst, src, size);
}
void *BackwardCopy(void *dst, const void *src, size_t size)
{
    return std::memmove(dst, src, size);
}

} // namespace TKit::Memory

#if !defined(TKIT_DISABLE_MEMORY_OVERRIDES) && !defined(TKIT_ENABLE_MIMALLOC)
void *operator new(const size_t size)
{
    return TKit::Memory::Allocate(size);
}
void *operator new[](const size_t size)
{
    return TKit::Memory::Allocate(size);
}
void *operator new(const size_t size, const std::align_val_t alignment)
{
    return TKit::Memory::AllocateAligned(size, static_cast<size_t>(alignment));
}
void *operator new[](const size_t size, const std::align_val_t alignment)
{
    return TKit::Memory::AllocateAligned(size, static_cast<size_t>(alignment));
}
void *operator new(const size_t size, const std::nothrow_t &) noexcept
{
    return TKit::Memory::Allocate(size);
}
void *operator new[](const size_t size, const std::nothrow_t &) noexcept
{
    return TKit::Memory::Allocate(size);
}

void operator delete(void *ptr) noexcept
{
    TKit::Memory::Deallocate(ptr);
}
void operator delete[](void *ptr) noexcept
{
    TKit::Memory::Deallocate(ptr);
}
void operator delete(void *ptr, std::align_val_t) noexcept
{
    TKit::Memory::DeallocateAligned(ptr);
}
void operator delete[](void *ptr, std::align_val_t) noexcept
{
    TKit::Memory::DeallocateAligned(ptr);
}
void operator delete(void *ptr, const std::nothrow_t &) noexcept
{
    TKit::Memory::Deallocate(ptr);
}
void operator delete[](void *ptr, const std::nothrow_t &) noexcept
{
    TKit::Memory::Deallocate(ptr);
}

void operator delete(void *ptr, size_t) noexcept
{
    TKit::Memory::Deallocate(ptr);
}
void operator delete[](void *ptr, size_t) noexcept
{
    TKit::Memory::Deallocate(ptr);
}
void operator delete(void *ptr, size_t, std::align_val_t) noexcept
{
    TKit::Memory::DeallocateAligned(ptr);
}
void operator delete[](void *ptr, size_t, std::align_val_t) noexcept
{
    TKit::Memory::DeallocateAligned(ptr);
}
void operator delete(void *ptr, size_t, const std::nothrow_t &) noexcept
{
    TKit::Memory::Deallocate(ptr);
}
void operator delete[](void *ptr, size_t, const std::nothrow_t &) noexcept
{
    TKit::Memory::Deallocate(ptr);
}

#endif
