#pragma once

#include "kit/core/logging.hpp"
#include "kit/core/non_copyable.hpp"

KIT_NAMESPACE_BEGIN

class StackAllocator final
{
    KIT_NON_COPYABLE(StackAllocator)
  public:
    struct Entry
    {
        Entry(std::byte *p_Data, usz p_Size) : Data(p_Data), Size(p_Size)
        {
        }
        std::byte *Data;
        usz Size;
    };

    explicit StackAllocator(usz p_Size) KIT_NOEXCEPT;
    ~StackAllocator() KIT_NOEXCEPT;

    StackAllocator(StackAllocator &&p_Other) noexcept;
    StackAllocator &operator=(StackAllocator &&p_Other) noexcept;

    const Entry &Top() const KIT_NOEXCEPT;
    template <typename T> T *Top() const KIT_NOEXCEPT
    {
        return reinterpret_cast<T *>(Top().Data);
    }

    void *Push(usz p_Size) KIT_NOEXCEPT;
    void Pop() KIT_NOEXCEPT;

    void *Allocate(usz p_Size) KIT_NOEXCEPT;
    void Deallocate([[maybe_unused]] const void *p_Ptr) KIT_NOEXCEPT;

    template <typename T> T *Push(const usz n = 1) KIT_NOEXCEPT
    {
        return static_cast<T *>(Push(n * sizeof(T)));
    }
    template <typename T> T *Allocate(const usz n = 1) KIT_NOEXCEPT
    {
        return static_cast<T *>(Allocate(n * sizeof(T)));
    }

    template <typename T, typename... Args> T *Create(Args &&...p_Args) KIT_NOEXCEPT
    {
        T *ptr = static_cast<T *>(Allocate(sizeof(T)));
        ::new (ptr) T(std::forward<Args>(p_Args)...);
        return ptr;
    }

    template <typename T, typename... Args> T *NConstruct(const usz p_N, Args &&...p_Args) KIT_NOEXCEPT
    {
        T *ptr = static_cast<T *>(Allocate(p_N * sizeof(T)));
        for (usz i = 0; i < p_N; ++i)
            ::new (&ptr[i]) T(std::forward<Args>(p_Args)...);
        return ptr;
    }

    template <typename T> void Destroy(T *p_Ptr) KIT_NOEXCEPT
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            KIT_ASSERT(!m_Entries.empty(), "Unable to deallocate because the stack allocator is empty");
            KIT_ASSERT(m_Entries.back().Data == p_Ptr,
                       "Elements must be deallocated in the reverse order they were allocated");

            const uptr n = m_Entries.back().Size / sizeof(T);
            for (usz i = 0; i < n; ++i)
                p_Ptr[i].~T();
        }
        Deallocate(p_Ptr);
    }

    usz Size() const KIT_NOEXCEPT;
    usz Allocated() const KIT_NOEXCEPT;
    usz Remaining() const KIT_NOEXCEPT;

    bool Empty() const KIT_NOEXCEPT;
    bool Full() const KIT_NOEXCEPT;
    bool Fits(usz p_Size) const KIT_NOEXCEPT;

  private:
    void deallocateBuffer() KIT_NOEXCEPT;

    std::byte *m_Buffer = nullptr;
    usz m_Size = 0;
    usz m_Allocated = 0;
    DynamicArray<Entry> m_Entries;
};

KIT_NAMESPACE_END