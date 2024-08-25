#pragma once

#include "kit/core/logging.hpp"
#include "kit/core/non_copyable.hpp"

KIT_NAMESPACE_BEGIN

// Stack allocator is a simple allocator that allocates memory in a stack-like fashion. It is useful for temporary
// allocations and allows many types of elements to coexist in a single contiguous chunk of memory. It is not
// thread-safe

// Thread safe considerations: This allocator requires precise ordering of allocations and deallocations. A
// multithreaded environment has the exact opposite property, so this allocator is not thread saf because *I think* it
// doesnt make sense to use it in a multithreaded environment as a shared variable

// TODO: There is a slight overhead when using Entry with a DynamicArray. Consider trying to reduce it (currently this
// allocator allocates/deallocates slower then the block allocator by aprox 1 ns per call, measured in my MacOS M1)
class KIT_API StackAllocator
{
    KIT_NON_COPYABLE(StackAllocator)
  public:
    struct Entry
    {
        // If asserts are enabled, I store the alignment offset so that I can assert the order of
        // allocations/deallocations is respected

        Entry(std::byte *p_Ptr, const usize p_Size, const usize p_AlignmentOffset)
            : Ptr(p_Ptr), Size(p_Size), AlignmentOffset(p_AlignmentOffset)
        {
        }
        std::byte *Ptr;
        usize Size;
        usize AlignmentOffset;
    };

    // The alignment parameter specifies the starting alignment of the whole block so that your first allocation will
    // not be padded in case you need specific alignment requirements for it, but it does not restrict the alignment of
    // the individual allocations at all. You can still specify alignments of 64 if you want when allocating
    explicit StackAllocator(usize p_Size, usize p_Alignment = 8) KIT_NOEXCEPT;
    ~StackAllocator() KIT_NOEXCEPT;

    StackAllocator(StackAllocator &&p_Other) noexcept;
    StackAllocator &operator=(StackAllocator &&p_Other) noexcept;

    // Push and Allocate do exactly the same, I just wantde to keep both APIs
    // Pop and Deallocate are *almost* the same. Deallocate requires the actual pointer to be passed. It is used mainly
    // for debugging purposes when asserts are enabled, to ensure proper deallocation order. Pop just mindlessly pops,
    // no asserts, no nothing

    // Alignment is set to 1, because in place allocations *should* allow any kind of alignment, including ones under 8
    // (not like malloc or posix_memalign). All of this in 64 bit systems
    void *Push(usize p_Size, usize p_Alignment = 1) KIT_NOEXCEPT;
    void Pop() KIT_NOEXCEPT;
    void Pop(usize p_N) KIT_NOEXCEPT;

    void *Allocate(usize p_Size, usize p_Alignment = 1) KIT_NOEXCEPT;
    void Deallocate([[maybe_unused]] const void *p_Ptr) KIT_NOEXCEPT;

    template <typename T> T *Push(const usize p_N = 1) KIT_NOEXCEPT
    {
        return static_cast<T *>(Push(p_N * sizeof(T), alignof(T)));
    }
    template <typename T> T *Allocate(const usize p_N = 1) KIT_NOEXCEPT
    {
        return static_cast<T *>(Allocate(p_N * sizeof(T), alignof(T)));
    }

    template <typename T, typename... Args> T *Create(Args &&...p_Args) KIT_NOEXCEPT
    {
        T *ptr = static_cast<T *>(Allocate(sizeof(T), alignof(T)));
        ::new (ptr) T(std::forward<Args>(p_Args)...);
        return ptr;
    }

    template <typename T, typename... Args> T *NConstruct(const usize p_N, Args &&...p_Args) KIT_NOEXCEPT
    {
        T *ptr = static_cast<T *>(Allocate(p_N * sizeof(T), alignof(T)));
        for (usize i = 0; i < p_N; ++i)
            ::new (&ptr[i]) T(std::forward<Args>(p_Args)...);
        return ptr;
    }

    template <typename T> void Destroy(T *p_Ptr) KIT_NOEXCEPT
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            KIT_ASSERT(!m_Entries.empty(), "Unable to deallocate because the stack allocator is empty");
            KIT_ASSERT(m_Entries.back().Ptr == reinterpret_cast<std::byte *>(p_Ptr),
                       "Elements must be deallocated in the reverse order they were allocated");

            const usize n = m_Entries.back().Size / sizeof(T);
            for (usize i = 0; i < n; ++i)
                p_Ptr[i].~T();
        }
        Deallocate(p_Ptr);
    }

    const Entry &Top() const KIT_NOEXCEPT;
    template <typename T> T *Top() KIT_NOEXCEPT
    {
        return reinterpret_cast<T *>(Top().Ptr);
    }

    usize Size() const KIT_NOEXCEPT;
    usize Allocated() const KIT_NOEXCEPT;
    usize Remaining() const KIT_NOEXCEPT;

    bool Belongs(const void *p_Ptr) const KIT_NOEXCEPT;
    bool Empty() const KIT_NOEXCEPT;
    bool Full() const KIT_NOEXCEPT;

  private:
    void deallocateBuffer() KIT_NOEXCEPT;

    std::byte *m_Buffer = nullptr;
    usize m_Size = 0;
    usize m_Remaining = 0;
    DynamicArray<Entry> m_Entries;
};

KIT_NAMESPACE_END