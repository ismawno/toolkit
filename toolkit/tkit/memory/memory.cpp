#include "tkit/core/pch.hpp"
#include "tkit/memory/memory.hpp"
#include "tkit/profiling/macros.hpp"
#include "tkit/preprocessor/utils.hpp"
#include <cstring>

namespace TKit::Memory
{
bool IsAligned(const void *p_Ptr, const size_t p_Alignment)
{
    const uptr addr = reinterpret_cast<uptr>(p_Ptr);
    return (addr & (p_Alignment - 1)) == 0;
}

bool IsAligned(const size_t p_Address, const size_t p_Alignment)
{
    return (p_Address & (p_Alignment - 1)) == 0;
}

void *Allocate(const size_t p_Size)
{
    void *ptr = std::malloc(p_Size);
    TKIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}

void Deallocate(void *p_Ptr)
{
    TKIT_PROFILE_MARK_DEALLOCATION(p_Ptr);
    std::free(p_Ptr);
}

void *AllocateAligned(const size_t p_Size, size_t p_Alignment)
{
    void *ptr = nullptr;
    p_Alignment = (p_Alignment + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
#ifdef TKIT_OS_WINDOWS
    ptr = _aligned_malloc(p_Size, p_Alignment);
#else
    int result = posix_memalign(&ptr, p_Alignment, p_Size);
    TKIT_UNUSED(result); // Sould do something with this at some point
#endif
    TKIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}

void DeallocateAligned(void *p_Ptr)
{
    TKIT_PROFILE_MARK_DEALLOCATION(p_Ptr);
#ifdef TKIT_OS_WINDOWS
    _aligned_free(p_Ptr);
#else
    std::free(p_Ptr);
#endif
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
