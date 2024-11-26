#pragma once

#include "kit/core/api.hpp"
#include "kit/core/alias.hpp"
#include <functional>

namespace TKit
{
// Could abstract this in some way (by defining those as function pointers) to allow for custom allocators
// For now, I'll leave it as it is

// TODO: Have an atomic counter to track allocations (conditionally with some macros)

/**
 * @brief Allocate a chunk of memory of a given size. Uses default malloc.
 *
 * It is here as a placeholder for future custom global allocators.
 *
 * @param p_Size The size of the memory to allocate.
 * @return void* A pointer to the allocated memory.
 */
TKIT_API void *Allocate(usize p_Size) noexcept;

/**
 * @brief Deallocate a chunk of memory. Uses default free.
 *
 * It is here as a placeholder for future custom global allocators.
 *
 * @param p_Ptr A pointer to the memory to deallocate.
 */
TKIT_API void Deallocate(void *p_Ptr) noexcept;

/**
 * @brief Allocate a chunk of memory of a given size with a given alignment.
 *
 * Uses the default platform-specific aligned allocation. It is here as a placeholder for future custom global
 * allocators.
 *
 * @param p_Size The size of the memory to allocate.
 * @param p_Alignment The alignment of the memory to allocate.
 * @return void* A pointer to the allocated memory.
 */
TKIT_API void *AllocateAligned(usize p_Size, usize p_Alignment) noexcept;

/**
 * @brief Deallocate a chunk of memory. Uses default platform-specific aligned deallocation.
 *
 * It is here as a placeholder for future custom global allocators.
 *
 * @param p_Ptr A pointer to the memory to deallocate.
 */
TKIT_API void DeallocateAligned(void *p_Ptr) noexcept;

} // namespace TKit

#ifdef TKIT_ENABLE_PROFILING

void *operator new(TKit::usize p_Size);
void *operator new[](TKit::usize p_Size);

void operator delete(void *p_Ptr) noexcept;
void operator delete[](void *p_Ptr) noexcept;

void *operator new(TKit::usize p_Size, const std::nothrow_t &) noexcept;
void *operator new[](TKit::usize p_Size, const std::nothrow_t &) noexcept;

void operator delete(void *p_Ptr, const std::nothrow_t &) noexcept;
void operator delete[](void *p_Ptr, const std::nothrow_t &) noexcept;

#endif