#include "tkit/core/pch.hpp"
#include "tkit/memory/arena_allocator.hpp"
#include "tkit/memory/memory.hpp"

namespace TKit
{
ArenaAllocator::ArenaAllocator(const usize p_Size) noexcept : m_Size(p_Size), m_Remaining(p_Size)
{
    m_Buffer = static_cast<std::byte *>(Allocate(p_Size));
}
ArenaAllocator::~ArenaAllocator() noexcept
{
    deallocateBuffer();
}

ArenaAllocator::ArenaAllocator(ArenaAllocator &&p_Other) noexcept
    : m_Buffer(p_Other.m_Buffer), m_Size(p_Other.m_Size), m_Remaining(p_Other.m_Remaining)
{
    p_Other.m_Buffer = nullptr;
    p_Other.m_Size = 0;
    p_Other.m_Remaining = 0;
}

ArenaAllocator &ArenaAllocator::operator=(ArenaAllocator &&p_Other) noexcept
{
    if (this != &p_Other)
    {
        deallocateBuffer();
        m_Buffer = p_Other.m_Buffer;
        m_Size = p_Other.m_Size;
        m_Remaining = p_Other.m_Remaining;

        p_Other.m_Buffer = nullptr;
        p_Other.m_Size = 0;
        p_Other.m_Remaining = 0;
    }
    return *this;
}

void *ArenaAllocator::Push(const usize p_Size, const usize p_Alignment) noexcept
{
    void *ptr = m_Buffer + (m_Size - m_Remaining);
    usize remaining = m_Remaining;

    std::byte *alignedPtr = static_cast<std::byte *>(std::align(p_Alignment, p_Size, ptr, remaining));
    TKIT_LOG_WARNING_IF(!alignedPtr, "[TOOLKIT] Arena allocator cannot fit {} bytes with {} alignment!", p_Size,
                        p_Alignment);
    if (!alignedPtr)
        return nullptr;
    TKIT_ASSERT(alignedPtr + p_Size <= m_Buffer + m_Size,
                "[TOOLKIT] Arena allocator failed to fit {} bytes with {} alignment! This is should not have triggered",
                p_Size, p_Alignment);

    m_Remaining = remaining - p_Size;
    return alignedPtr;
}

void ArenaAllocator::Reset() noexcept
{
    m_Remaining = m_Size;
}

usize ArenaAllocator::GetSize() const noexcept
{
    return m_Size;
}
usize ArenaAllocator::GetAllocated() const noexcept
{
    return m_Size - m_Remaining;
}
usize ArenaAllocator::GetRemaining() const noexcept
{
    return m_Remaining;
}

bool ArenaAllocator::Belongs(const void *p_Ptr) const noexcept
{
    const std::byte *ptr = reinterpret_cast<const std::byte *>(p_Ptr);
    return ptr >= m_Buffer && ptr < m_Buffer + (m_Size - m_Remaining);
}

bool ArenaAllocator::IsEmpty() const noexcept
{
    return m_Remaining == m_Size;
}

bool ArenaAllocator::IsFull() const noexcept
{
    return m_Remaining == 0;
}

void ArenaAllocator::deallocateBuffer() noexcept
{
    if (!m_Buffer)
        return;
    TKIT_LOG_WARNING_IF(
        m_Remaining != m_Size,
        "[TOOLKIT] Deallocating a Arena allocator with active allocations. Consider calling Pop() until the "
        "allocator is empty. If the elements are not trivially destructible, you will have to call "
        "Destroy() for each element (this deallocation will not call the destructor)");
    Deallocate(m_Buffer);
    m_Buffer = nullptr;
    m_Size = 0;
    m_Remaining = 0;
}
} // namespace TKit