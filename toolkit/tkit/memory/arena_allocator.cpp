#include "tkit/core/pch.hpp"
#include "tkit/memory/arena_allocator.hpp"
#include "tkit/utils/debug.hpp"
#include "tkit/utils/bit.hpp"

namespace TKit
{
ArenaAllocator::ArenaAllocator(void *buffer, const usize capacity, const usize alignment)
    : m_Buffer(static_cast<std::byte *>(buffer)), m_Capacity(capacity), m_Alignment(alignment), m_Provided(true)
{
    TKIT_ASSERT(IsPowerOfTwo(alignment),
                "[TOOLKIT][ARENA-ALLOC] Alignment must be a power of 2, but the value is {}", alignment);
    TKIT_ASSERT(IsAligned(buffer, alignment),
                "[TOOLKIT][ARENA-ALLOC] Provided buffer must be aligned to the given alignment of {}", alignment);
}
ArenaAllocator::ArenaAllocator(const usize capacity, const usize alignment)
    : m_Capacity(capacity), m_Alignment(alignment), m_Provided(false)
{
    TKIT_ASSERT(IsPowerOfTwo(alignment),
                "[TOOLKIT][ARENA-ALLOC] Alignment must be a power of 2, but the value is {}", alignment);
    m_Buffer = static_cast<std::byte *>(AllocateAligned(static_cast<size_t>(capacity), alignment));
    TKIT_ASSERT(m_Buffer, "[TOOLKIT][ARENA-ALLOC] Failed to allocate memory");
}
ArenaAllocator::~ArenaAllocator()
{
    deallocateBuffer();
}

ArenaAllocator::ArenaAllocator(ArenaAllocator &&other)
    : m_Buffer(other.m_Buffer), m_Top(other.m_Top), m_Capacity(other.m_Capacity), m_Alignment(other.m_Alignment),
      m_Provided(other.m_Provided)
{
    other.m_Buffer = nullptr;
    other.m_Top = 0;
    other.m_Capacity = 0;
    other.m_Alignment = 0;
    other.m_Provided = false;
}

ArenaAllocator &ArenaAllocator::operator=(ArenaAllocator &&other)
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

void *ArenaAllocator::Allocate(const usize size)
{
    TKIT_ASSERT(size != 0, "[TOOLKIT][ARENA-ALLOC] Cannot allocate 0 bytes");
    const usize asize = NextAlignedSize(size, m_Alignment);
    if (m_Top + asize > m_Capacity)
    {
        TKIT_LOG_WARNING(
            "[TOOLKIT][ARENA-ALLOC] Allocator ran out of memory while trying to allocate {} bytes (only {} remaining)",
            asize, m_Capacity - m_Top);
        return nullptr;
    }

    std::byte *ptr = m_Buffer + m_Top;
    m_Top += asize;
    TKIT_ASSERT(IsAligned(ptr, m_Alignment),
                "[TOOLKIT][ARENA-ALLOC] Allocated memory is not aligned to specified alignment");
    return ptr;
}

void ArenaAllocator::deallocateBuffer()
{
    if (!m_Buffer || m_Provided)
        return;
    // TKIT_LOG_WARNING_IF(
    //     m_Top != 0,
    //     "[TOOLKIT][ARENA-ALLOC] Deallocating an arena allocator with active allocations. If the elements are not "
    //     "trivially destructible, you will have to call "
    //     "Destroy() for each element to avoid undefined behaviour (this deallocation will not call the destructor)");
    DeallocateAligned(m_Buffer);
}
} // namespace TKit
