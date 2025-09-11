#include "tkit/core/pch.hpp"
#include "tkit/memory/stack_allocator.hpp"
#include "tkit/memory/memory.hpp"

namespace TKit
{
StackAllocator::StackAllocator(void *p_Buffer, const usize p_Size)
    : m_Buffer(static_cast<std::byte *>(p_Buffer)), m_Size(p_Size), m_Remaining(p_Size), m_Provided(true)
{
}

StackAllocator::StackAllocator(const usize p_Size, const usize p_Alignment)
    : m_Size(p_Size), m_Remaining(p_Size), m_Provided(false)
{
    m_Buffer = static_cast<std::byte *>(Memory::AllocateAligned(static_cast<size_t>(p_Size), p_Alignment));
    TKIT_ASSERT(m_Buffer, "[TOOLKIT][STACK-ALLOC] Failed to allocate memory");
}

StackAllocator::~StackAllocator()
{
    deallocateBuffer();
}

StackAllocator::StackAllocator(StackAllocator &&p_Other)
    : m_Entries(std::move(p_Other.m_Entries)), m_Buffer(p_Other.m_Buffer), m_Size(p_Other.GetSize()),
      m_Remaining(p_Other.m_Remaining), m_Provided(p_Other.m_Provided)

{
    p_Other.m_Buffer = nullptr;
    p_Other.m_Size = 0;
    p_Other.m_Remaining = 0;
    p_Other.m_Entries.Clear();
}

StackAllocator &StackAllocator::operator=(StackAllocator &&p_Other)
{
    if (this != &p_Other)
    {
        deallocateBuffer();
        m_Buffer = p_Other.m_Buffer;
        m_Size = p_Other.GetSize();
        m_Remaining = p_Other.m_Remaining;
        m_Entries = std::move(p_Other.m_Entries);
        m_Provided = p_Other.m_Provided;

        p_Other.m_Buffer = nullptr;
        p_Other.m_Size = 0;
        p_Other.m_Remaining = 0;
        p_Other.m_Entries.Clear();
    }
    return *this;
}

void *StackAllocator::Allocate(const usize p_Size, const usize p_Alignment)
{
    void *ptr = m_Entries.IsEmpty() ? m_Buffer : m_Entries.GetBack().Ptr + m_Entries.GetBack().Size;
    size_t remaining = static_cast<size_t>(m_Remaining);

    std::byte *alignedPtr = static_cast<std::byte *>(std::align(p_Alignment, p_Size, ptr, remaining));
    // TKIT_LOG_WARNING_IF(!alignedPtr, "[TOOLKIT][STACK-ALLOC] Stack allocator cannot fit {} bytes with {} alignment!",
    //                     p_Size, p_Alignment);
    if (!alignedPtr)
        return nullptr;
    TKIT_ASSERT(alignedPtr + p_Size <= m_Buffer + m_Size,
                "[TOOLKIT][STACK-ALLOC] Stack allocator failed to fit {} bytes with {} alignment! This is should not "
                "have triggered",
                p_Size, p_Alignment);
    TKIT_ASSERT(Memory::IsAligned(alignedPtr, p_Alignment),
                "[TOOLKIT][STACK-ALLOC] Aligned pointer is not aligned to the requested alignment");

    const usize offset = m_Remaining - static_cast<usize>(remaining);
    m_Remaining = static_cast<usize>(remaining) - p_Size;

    m_Entries.Append(alignedPtr, p_Size, offset);
    return alignedPtr;
}

void StackAllocator::Deallocate([[maybe_unused]] const void *p_Ptr)
{
    TKIT_ASSERT(p_Ptr, "[TOOLKIT][STACK-ALLOC] Cannot deallocate a null pointer");
    TKIT_ASSERT(!m_Entries.IsEmpty(),
                "[TOOLKIT][STACK-ALLOC] Unable to deallocate because the stack allocator is IsEmpty");
    TKIT_ASSERT(m_Entries.GetBack().Ptr == p_Ptr,
                "[TOOLKIT][STACK-ALLOC] Elements must be deallocated in the reverse order they were allocated");
    m_Remaining += m_Entries.GetBack().Size + m_Entries.GetBack().AlignmentOffset;
    m_Entries.Pop();
}
void StackAllocator::Deallocate()
{
    TKIT_ASSERT(!m_Entries.IsEmpty(),
                "[TOOLKIT][STACK-ALLOC] Unable to deallocate because the stack allocator is IsEmpty");
    m_Remaining += m_Entries.GetBack().Size + m_Entries.GetBack().AlignmentOffset;
    m_Entries.Pop();
}

const StackAllocator::Entry &StackAllocator::Top() const
{
    TKIT_ASSERT(!m_Entries.IsEmpty(), "[TOOLKIT][STACK-ALLOC] No elements in the stack allocator");
    return m_Entries.GetBack();
}

usize StackAllocator::GetSize() const
{
    return m_Size;
}
usize StackAllocator::GetAllocated() const
{
    return m_Size - m_Remaining;
}
usize StackAllocator::GetRemaining() const
{
    return m_Remaining;
}

bool StackAllocator::Belongs(const void *p_Ptr) const
{
    if (m_Entries.IsEmpty())
        return false;
    const std::byte *ptr = reinterpret_cast<const std::byte *>(p_Ptr);
    return ptr >= m_Buffer && ptr < m_Entries.GetBack().Ptr + m_Entries.GetBack().Size;
}

bool StackAllocator::IsEmpty() const
{
    return m_Remaining == m_Size;
}

bool StackAllocator::IsFull() const
{
    return m_Remaining == 0;
}

void StackAllocator::deallocateBuffer()
{
    if (!m_Buffer || m_Provided)
        return;
    TKIT_LOG_WARNING_IF(
        !m_Entries.IsEmpty(),
        "[TOOLKIT][STACK-ALLOC] Deallocating a stack allocator with active allocations. If the elements are not "
        "trivially destructible, you will have to call "
        "Destroy() for each element to avoid undefined behaviour (this deallocation will not call the destructor)");
    Memory::DeallocateAligned(m_Buffer);
    m_Buffer = nullptr;
    m_Size = 0;
    m_Remaining = 0;
    m_Entries.Clear();
}
} // namespace TKit
