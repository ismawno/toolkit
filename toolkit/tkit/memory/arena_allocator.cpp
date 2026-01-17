#include "tkit/core/pch.hpp"
#include "tkit/memory/arena_allocator.hpp"
#include "tkit/utils/debug.hpp"
#include "tkit/utils/bit.hpp"

namespace TKit
{
ArenaAllocator::ArenaAllocator(void *p_Buffer, const usize p_Capacity, const usize p_Alignment)
    : m_Buffer(static_cast<std::byte *>(p_Buffer)), m_Capacity(p_Capacity), m_Alignment(p_Alignment), m_Provided(true)
{
    TKIT_ASSERT(Bit::IsPowerOfTwo(p_Alignment),
                "[TOOLKIT][ARENA-ALLOC] Alignment must be a power of 2, but the value is {}", p_Alignment);
    TKIT_ASSERT(Memory::IsAligned(p_Buffer, p_Alignment),
                "[TOOLKIT][ARENA-ALLOC] Provided buffer must be aligned to the given alignment of {}", p_Alignment);
}
ArenaAllocator::ArenaAllocator(const usize p_Capacity, const usize p_Alignment)
    : m_Capacity(p_Capacity), m_Alignment(p_Alignment), m_Provided(false)
{
    TKIT_ASSERT(Bit::IsPowerOfTwo(p_Alignment),
                "[TOOLKIT][ARENA-ALLOC] Alignment must be a power of 2, but the value is {}", p_Alignment);
    m_Buffer = static_cast<std::byte *>(Memory::AllocateAligned(static_cast<size_t>(p_Capacity), p_Alignment));
    TKIT_ASSERT(m_Buffer, "[TOOLKIT][ARENA-ALLOC] Failed to allocate memory");
}
ArenaAllocator::~ArenaAllocator()
{
    deallocateBuffer();
}

ArenaAllocator::ArenaAllocator(ArenaAllocator &&p_Other)
    : m_Buffer(p_Other.m_Buffer), m_Top(p_Other.m_Top), m_Capacity(p_Other.m_Capacity),
      m_Alignment(p_Other.m_Alignment), m_Provided(p_Other.m_Provided)
{
    p_Other.m_Buffer = nullptr;
    p_Other.m_Top = 0;
    p_Other.m_Capacity = 0;
    p_Other.m_Alignment = 0;
    p_Other.m_Provided = false;
}

ArenaAllocator &ArenaAllocator::operator=(ArenaAllocator &&p_Other)
{
    if (this != &p_Other)
    {
        deallocateBuffer();
        m_Buffer = p_Other.m_Buffer;
        m_Top = p_Other.m_Top;
        m_Capacity = p_Other.m_Capacity;
        m_Alignment = p_Other.m_Alignment;
        m_Provided = p_Other.m_Provided;

        p_Other.m_Buffer = nullptr;
        p_Other.m_Top = 0;
        p_Other.m_Capacity = 0;
        p_Other.m_Alignment = 0;
        p_Other.m_Provided = false;
    }
    return *this;
}

void *ArenaAllocator::Allocate(const usize p_Size)
{
    TKIT_ASSERT(p_Size != 0, "[TOOLKIT][ARENA-ALLOC] Cannot allocate 0 bytes");
    const usize size = Memory::NextAlignedSize(p_Size, m_Alignment);
    if (m_Top + size > m_Capacity)
    {
        TKIT_LOG_WARNING(
            "[TOOLKIT][ARENA-ALLOC] Allocator ran out of memory while trying to allocate {} bytes (only {} remaining)",
            size, m_Capacity - m_Top);
        return nullptr;
    }

    std::byte *ptr = m_Buffer + m_Top;
    m_Top += size;
    TKIT_ASSERT(Memory::IsAligned(ptr, m_Alignment),
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
    Memory::DeallocateAligned(m_Buffer);
}
} // namespace TKit
