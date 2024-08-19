#pragma once

#include "kit/core/core.hpp"
#include "kit/core/alias.hpp"
#include <functional>

KIT_NAMESPACE_BEGIN

// Could abstract this in some way (by defining those as function pointers) to allow for custom allocators
// For now, I'll leave it as it is

// TODO: Have an atomic counter to track allocations (conditionally with some macros)

KIT_API void *Allocate(usize p_Size) KIT_NOEXCEPT;
KIT_API void Deallocate(void *p_Ptr) KIT_NOEXCEPT;

KIT_API void *AllocateAligned(usize p_Size, usize p_Alignment) KIT_NOEXCEPT;
KIT_API void DeallocateAligned(void *p_Ptr) KIT_NOEXCEPT;

// This is a function that I have here just in case I (for whatever reasong) need to align a size. Which, to my
// knowledge, should never happen (I have yet to see a case where the size of a type is not a multiple of its alignment)
// It is currently being used by the block allocator, but it should always return sizeof(T)
template <typename T> consteval usize AlignedSize() KIT_NOEXCEPT
{
    constexpr usize remainder = sizeof(T) % alignof(T);
    if constexpr (remainder == 0)
        return sizeof(T);
    else
        return sizeof(T) + alignof(T) - remainder;
}

KIT_NAMESPACE_END