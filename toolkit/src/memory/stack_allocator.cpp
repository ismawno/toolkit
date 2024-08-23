#include "core/pch.hpp"
#include "kit/memory/stack_allocator.hpp"
#include "kit/memory/memory.hpp"

KIT_NAMESPACE_BEGIN

StackAllocator::StackAllocator(const usize p_Size, const usize p_Alignment) KIT_NOEXCEPT : m_Size(p_Size),
                                                                                           m_Remaining(p_Size)
{
    m_Buffer = static_cast<std::byte *>(AllocateAligned(p_Size, p_Alignment));
    m_Entries.reserve(p_Size / sizeof(Entry));
}

StackAllocator::~StackAllocator() KIT_NOEXCEPT
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

void *StackAllocator::Push(const usize p_Size, const usize p_Alignment) KIT_NOEXCEPT
{
    void *ptr = m_Entries.empty() ? m_Buffer : m_Entries.back().Ptr + m_Entries.back().Size;
    usize remaining = m_Remaining;

    std::byte *alignedPtr = static_cast<std::byte *>(std::align(p_Alignment, p_Size, ptr, remaining));
    KIT_LOG_WARNING_IF(!alignedPtr, "Stack allocator cannot fit {} bytes with {} alignment!", p_Size, p_Alignment);
    if (!alignedPtr)
        return nullptr;

    const usize offset = m_Remaining - remaining;
    m_Remaining = remaining - p_Size;

    m_Entries.emplace_back(alignedPtr, p_Size, offset);
    return alignedPtr;
}

void StackAllocator::Pop() KIT_NOEXCEPT
{
    KIT_LOG_WARNING_IF(m_Entries.empty(), "Popping from an empty allocator");
    m_Remaining += m_Entries.back().Size + m_Entries.back().AlignmentOffset;
    m_Entries.pop_back();
}

void StackAllocator::Pop(const usize p_N) KIT_NOEXCEPT
{
    KIT_LOG_WARNING_IF(m_Entries.size() < p_N, "Popping more elements than the allocator has");
    for (usize i = 0; i < p_N; ++i)
        Pop();
}

void *StackAllocator::Allocate(const usize p_Size, const usize p_Alignment) KIT_NOEXCEPT
{
    return Push(p_Size, p_Alignment);
}

void StackAllocator::Deallocate([[maybe_unused]] const void *p_Ptr) KIT_NOEXCEPT
{
    KIT_ASSERT(!m_Entries.empty(), "Unable to deallocate because the stack allocator is empty");
    KIT_ASSERT(m_Entries.back().Ptr == p_Ptr, "Elements must be deallocated in the reverse order they were allocated");
    Pop();
}

const StackAllocator::Entry &StackAllocator::Top() const KIT_NOEXCEPT
{
    KIT_ASSERT(!m_Entries.empty(), "No elements in the stack allocator");
    return m_Entries.back();
}

usize StackAllocator::Size() const KIT_NOEXCEPT
{
    return m_Size;
}
usize StackAllocator::Allocated() const KIT_NOEXCEPT
{
    return m_Size - m_Remaining;
}
usize StackAllocator::Remaining() const KIT_NOEXCEPT
{
    return m_Remaining;
}

bool StackAllocator::Empty() const KIT_NOEXCEPT
{
    return m_Remaining == m_Size;
}

bool StackAllocator::Full() const KIT_NOEXCEPT
{
    return m_Remaining == 0;
}

void StackAllocator::deallocateBuffer() KIT_NOEXCEPT
{
    if (!m_Buffer)
        return;
    KIT_LOG_WARNING_IF(!m_Entries.empty(),
                       "Deallocating a stack allocator with active allocations. Consider calling Pop() until the "
                       "allocator is empty. If the elements are not trivially destructible, you will have to call "
                       "Destroy() for each element (this deallocation will not call the destructor)");
    DeallocateAligned(m_Buffer);
    m_Buffer = nullptr;
    m_Size = 0;
    m_Remaining = 0;
    m_Entries.clear();
}

KIT_NAMESPACE_END