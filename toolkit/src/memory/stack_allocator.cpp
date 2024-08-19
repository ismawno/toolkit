#include "core/pch.hpp"
#include "kit/memory/stack_allocator.hpp"

KIT_NAMESPACE_BEGIN

StackAllocator::StackAllocator(const usize p_Size) KIT_NOEXCEPT : m_Buffer(new std::byte[p_Size]), m_Size(p_Size)
{
    m_Entries.reserve(p_Size / sizeof(Entry));
}

StackAllocator::~StackAllocator() KIT_NOEXCEPT
{
    deallocateBuffer();
}

StackAllocator::StackAllocator(StackAllocator &&p_Other) noexcept
    : m_Buffer(p_Other.m_Buffer), m_Size(p_Other.m_Size), m_Allocated(p_Other.m_Allocated),
      m_Entries(std::move(p_Other.m_Entries))
{
    p_Other.m_Buffer = nullptr;
    p_Other.m_Size = 0;
    p_Other.m_Allocated = 0;
    p_Other.m_Entries.clear();
}

StackAllocator &StackAllocator::operator=(StackAllocator &&p_Other) noexcept
{
    if (this != &p_Other)
    {
        deallocateBuffer();
        m_Buffer = p_Other.m_Buffer;
        m_Size = p_Other.m_Size;
        m_Allocated = p_Other.m_Allocated;
        m_Entries = std::move(p_Other.m_Entries);

        p_Other.m_Buffer = nullptr;
        p_Other.m_Size = 0;
        p_Other.m_Allocated = 0;
        p_Other.m_Entries.clear();
    }
    return *this;
}

void *StackAllocator::Push(usize p_Size) KIT_NOEXCEPT
{
    KIT_ASSERT(Fits(p_Size), "Stack allocator cannot fit {} bytes!", p_Size);
    std::byte *ptr = m_Entries.empty() ? m_Buffer : m_Entries.back().Data + m_Entries.back().Size;

    m_Entries.emplace_back(ptr, p_Size);
    m_Allocated += p_Size;
    return ptr;
}

void StackAllocator::Pop() KIT_NOEXCEPT
{
    KIT_LOG_WARNING_IF(m_Entries.empty(), "Popping from an empty allocator");
    m_Allocated -= m_Entries.back().Size;
    m_Entries.pop_back();
}

void *StackAllocator::Allocate(usize p_Size) KIT_NOEXCEPT
{
    return Push(p_Size);
}

void StackAllocator::Deallocate([[maybe_unused]] const void *p_Ptr) KIT_NOEXCEPT
{
    KIT_ASSERT(!m_Entries.empty(), "Unable to deallocate because the stack allocator is empty");
    KIT_ASSERT(m_Entries.back().Data == p_Ptr, "Elements must be deallocated in the reverse order they were allocated");
    Pop();
}

const StackAllocator::Entry &StackAllocator::Top() const KIT_NOEXCEPT
{
    KIT_ASSERT(!m_Entries.empty(), "Stack allocator is empty");
    return m_Entries.back();
}

usize StackAllocator::Size() const KIT_NOEXCEPT
{
    return m_Size;
}
usize StackAllocator::Allocated() const KIT_NOEXCEPT
{
    return m_Allocated;
}
usize StackAllocator::Remaining() const KIT_NOEXCEPT
{
    return m_Size - m_Allocated;
}

bool StackAllocator::Empty() const KIT_NOEXCEPT
{
    return m_Allocated == 0;
}

bool StackAllocator::Full() const KIT_NOEXCEPT
{
    return m_Allocated == m_Size;
}

bool StackAllocator::Fits(usize p_Size) const KIT_NOEXCEPT
{
    return m_Allocated + p_Size <= m_Size;
}

void StackAllocator::deallocateBuffer() KIT_NOEXCEPT
{
    if (!m_Buffer)
        return;
    delete[] m_Buffer;
    m_Buffer = nullptr;
    m_Size = 0;
    m_Allocated = 0;
    m_Entries.clear();
}

KIT_NAMESPACE_END