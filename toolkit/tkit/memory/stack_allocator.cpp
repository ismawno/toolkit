#include "tkit/core/pch.hpp"
#include "tkit/memory/stack_allocator.hpp"
#include "tkit/memory/memory.hpp"
#include "tkit/utils/bit.hpp"

namespace TKit
{
StackAllocator::StackAllocator(void *buffer, const usize capacity, const usize alignment)
    : m_Buffer(static_cast<std::byte *>(buffer)), m_Capacity(capacity), m_Alignment(alignment), m_Provided(true)
{
    TKIT_ASSERT(IsPowerOfTwo(alignment), "[TOOLKIT][STACK-ALLOC] Alignment must be a power of 2, but the value is {}",
                alignment);
    TKIT_ASSERT(IsAligned(buffer, alignment),
                "[TOOLKIT][STACK-ALLOC] Provided buffer must be aligned to the given alignment of {}", alignment);
}

StackAllocator::StackAllocator(const usize capacity, const usize alignment)
    : m_Capacity(capacity), m_Alignment(alignment), m_Provided(false)
{
    TKIT_ASSERT(IsPowerOfTwo(alignment), "[TOOLKIT][STACK-ALLOC] Alignment must be a power of 2, but the value is {}",
                alignment);
    m_Buffer = static_cast<std::byte *>(AllocateAligned(static_cast<size_t>(capacity), alignment));
    TKIT_ASSERT(m_Buffer, "[TOOLKIT][STACK-ALLOC] Failed to allocate {:L} bytes of memory aligned to {:L} bytes",
                capacity, alignment);
}

StackAllocator::~StackAllocator()
{
    deallocateBuffer();
}

StackAllocator::StackAllocator(StackAllocator &&other)
    : m_Buffer(other.m_Buffer), m_Top(other.m_Top), m_Capacity(other.m_Capacity), m_Alignment(other.m_Alignment),
      m_Provided(other.m_Provided)

{
    other.m_Buffer = nullptr;
    other.m_Top = 0;
    other.m_Capacity = 0;
    other.m_Alignment = 0;
    other.m_Provided = false;
}

StackAllocator &StackAllocator::operator=(StackAllocator &&other)
{
    if (this != &other)
    {
        deallocateBuffer();
        m_Buffer = other.m_Buffer;
        m_Top = other.m_Top;
        m_Capacity = other.m_Capacity;
        m_Alignment = other.m_Alignment;
        m_Provided = other.m_Provided;

        other.m_Buffer = nullptr;
        other.m_Top = 0;
        other.m_Capacity = 0;
        other.m_Alignment = 0;
        other.m_Provided = false;
    }
    return *this;
}

void *StackAllocator::Allocate(const usize size)
{
    TKIT_ASSERT(size != 0, "[TOOLKIT][STACK-ALLOC] Cannot allocate 0 bytes");
    const usize asize = NextAlignedSize(size, m_Alignment);
    if (m_Top + asize > m_Capacity)
    {
        TKIT_LOG_WARNING("[TOOLKIT][STACK-ALLOC] Allocator ran out of memory while trying to allocate {:L} bytes (only "
                         "{:L} remaining)",
                         asize, m_Capacity - m_Top);
        return nullptr;
    }

    std::byte *ptr = m_Buffer + m_Top;
    m_Top += asize;
    TKIT_ASSERT(IsAligned(ptr, m_Alignment),
                "[TOOLKIT][STACK-ALLOC] Allocated memory is not aligned to specified alignment");
    return ptr;
}

void StackAllocator::Deallocate([[maybe_unused]] const void *ptr, const usize size)
{
    TKIT_ASSERT(ptr, "[TOOLKIT][STACK-ALLOC] Cannot deallocate a null pointer");
    TKIT_ASSERT(m_Top != 0, "[TOOLKIT][STACK-ALLOC] Unable to deallocate because the stack allocator is empty");
    m_Top -= NextAlignedSize(size, m_Alignment);
    TKIT_ASSERT(m_Buffer + m_Top == ptr,
                "[TOOLKIT][STACK-ALLOC] Elements must be deallocated in the reverse order they were allocated");
}

void StackAllocator::deallocateBuffer()
{
    if (!m_Buffer || m_Provided)
        return;
    TKIT_LOG_WARNING_IF(
        m_Top != 0,
        "[TOOLKIT][STACK-ALLOC] Deallocating a stack allocator with active allocations. If the elements are not "
        "trivially destructible, you will have to call "
        "Destroy() for each element to avoid undefined behaviour (this deallocation will not call the destructor)");
    DeallocateAligned(m_Buffer);
}
} // namespace TKit
