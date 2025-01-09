#include "tkit/core/pch.hpp"
#include "tkit/memory/stack_allocator.hpp"
#include "tkit/memory/memory.hpp"

namespace TKit
{
StackAllocator::StackAllocator(const usize p_Size) noexcept : m_Size(p_Size), m_Remaining(p_Size)
{
    m_Buffer = static_cast<std::byte *>(Allocate(p_Size));
    m_Entries.reserve(p_Size / sizeof(Entry));
}

StackAllocator::~StackAllocator() noexcept
{
    deallocateBuffer();
}

StackAllocator::StackAllocator(StackAllocator &&p_Other) noexcept
    : m_Buffer(p_Other.m_Buffer), m_Size(p_Other.m_Size), m_Remaining(p_Other.m_Remaining),
      m_Entries(std::move(p_Other.m_Entries))
{
    p_Other.m_Buffer = nullptr;
    p_Other.m_Size = 0;
    p_Other.m_Remaining = 0;
    p_Other.m_Entries.clear();
}

StackAllocator &StackAllocator::operator=(StackAllocator &&p_Other) noexcept
{
    if (this != &p_Other)
    {
        deallocateBuffer();
        m_Buffer = p_Other.m_Buffer;
        m_Size = p_Other.m_Size;
        m_Remaining = p_Other.m_Remaining;
        m_Entries = std::move(p_Other.m_Entries);

        p_Other.m_Buffer = nullptr;
        p_Other.m_Size = 0;
        p_Other.m_Remaining = 0;
        p_Other.m_Entries.clear();
    }
    return *this;
}

void *StackAllocator::Push(const usize p_Size, const usize p_Alignment) noexcept
{
    void *ptr = m_Entries.empty() ? m_Buffer : m_Entries.back().Ptr + m_Entries.back().Size;
    usize remaining = m_Remaining;

    std::byte *alignedPtr = static_cast<std::byte *>(std::align(p_Alignment, p_Size, ptr, remaining));
    TKIT_LOG_WARNING_IF(!alignedPtr, "[TOOLKIT] Stack allocator cannot fit {} bytes with {} alignment!", p_Size,
                        p_Alignment);
    if (!alignedPtr)
        return nullptr;
    TKIT_ASSERT(alignedPtr + p_Size <= m_Buffer + m_Size,
                "[TOOLKIT] Stack allocator failed to fit {} bytes with {} alignment! This is should not have triggered",
                p_Size, p_Alignment);

    const usize offset = m_Remaining - remaining;
    m_Remaining = remaining - p_Size;

    m_Entries.emplace_back(alignedPtr, p_Size, offset);
    return alignedPtr;
}

void StackAllocator::Pop() noexcept
{
    TKIT_LOG_WARNING_IF(m_Entries.empty(), "[TOOLKIT] Popping from an empty allocator");
    m_Remaining += m_Entries.back().Size + m_Entries.back().AlignmentOffset;
    m_Entries.pop_back();
}

void *StackAllocator::Allocate(const usize p_Size, const usize p_Alignment) noexcept
{
    return Push(p_Size, p_Alignment);
}

void StackAllocator::Deallocate([[maybe_unused]] const void *p_Ptr) noexcept
{
    TKIT_ASSERT(!m_Entries.empty(), "[TOOLKIT] Unable to deallocate because the stack allocator is empty");
    TKIT_ASSERT(m_Entries.back().Ptr == p_Ptr,
                "[TOOLKIT] Elements must be deallocated in the reverse order they were allocated");
    Pop();
}

const StackAllocator::Entry &StackAllocator::Top() const noexcept
{
    TKIT_ASSERT(!m_Entries.empty(), "[TOOLKIT] No elements in the stack allocator");
    return m_Entries.back();
}

usize StackAllocator::GetSize() const noexcept
{
    return m_Size;
}
usize StackAllocator::GetAllocated() const noexcept
{
    return m_Size - m_Remaining;
}
usize StackAllocator::GetRemaining() const noexcept
{
    return m_Remaining;
}

bool StackAllocator::Belongs(const void *p_Ptr) const noexcept
{
    if (m_Entries.empty())
        return false;
    const std::byte *ptr = reinterpret_cast<const std::byte *>(p_Ptr);
    return ptr >= m_Buffer && ptr < m_Entries.back().Ptr + m_Entries.back().Size;
}

bool StackAllocator::IsEmpty() const noexcept
{
    return m_Remaining == m_Size;
}

bool StackAllocator::IsFull() const noexcept
{
    return m_Remaining == 0;
}

void StackAllocator::deallocateBuffer() noexcept
{
    if (!m_Buffer)
        return;
    TKIT_LOG_WARNING_IF(
        !m_Entries.empty(),
        "[TOOLKIT] Deallocating a stack allocator with active allocations. Consider calling Pop() until the "
        "allocator is empty. If the elements are not trivially destructible, you will have to call "
        "Destroy() for each element (this deallocation will not call the destructor)");
    Deallocate(m_Buffer);
    m_Buffer = nullptr;
    m_Size = 0;
    m_Remaining = 0;
    m_Entries.clear();
}
} // namespace TKit