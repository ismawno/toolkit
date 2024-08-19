#pragma once

#include "kit/core/logging.hpp"
#include "kit/core/concepts.hpp"
#include "kit/memory/block_allocator.hpp"
#include <memory>
#include <atomic>

KIT_NAMESPACE_BEGIN

// For now, Scope is a disguised unique_ptr
template <typename T> using Scope = std::unique_ptr<T>;

// I really tried to use this concept, but msvc wont fucking let me
// template <typename T, template <typename> typename RC>
// concept RCounted = std::is_base_of_v<RC<typename T::CountedType>, T>;

// This is a small homemade implementation of a reference counter to avoid the shared_ptr's allocations overhead.
// This way, the reference counter is stored in the object itself. Any object that wishes to be reference counted
// should inherit from this class
template <typename T> class RefCounted
{
  public:
    using CountedType = T;
    // No initialization of refcount. Refcount adds/removes are handled by Ref
    RefCounted() KIT_NOEXCEPT = default;

    /* COPY-MOVE OPERATIONS */
    // Refcount should not be copied. When doing *ptr1 = *ptr2, we only want to transfer the user's data
    RefCounted(const RefCounted &) KIT_NOEXCEPT
    {
    }
    RefCounted(RefCounted &&) noexcept
    {
    }

    RefCounted &operator=(const RefCounted &) KIT_NOEXCEPT
    {
        return *this;
    }
    RefCounted &operator=(RefCounted &&) noexcept
    {
        return *this;
    }

    KIT_WARNING_IGNORE_PUSH
    KIT_CLANG_WARNING_IGNORE("-Wexceptions")
    KIT_GCC_WARNING_IGNORE("-Wterminate")
    KIT_MSVC_WARNING_IGNORE(4297)
    ~RefCounted() KIT_NOEXCEPT
    {
        KIT_ASSERT(m_RefCount.load(std::memory_order_relaxed) == 0, "RefCounted object deleted with non-zero refcount");
    }
    KIT_WARNING_IGNORE_POP

    u32 RefCount() const KIT_NOEXCEPT
    {
        return m_RefCount.load(std::memory_order_relaxed);
    }

  protected:
    // User may want to have control over the objects's destruction, so they can implement this method instead of using
    // the default
    void selfDestruct() const KIT_NOEXCEPT
    {
        delete static_cast<const T *>(this);
    }

  private:
    void increaseRef() const KIT_NOEXCEPT
    {
        m_RefCount.fetch_add(1, std::memory_order_relaxed);
    }

    void decreaseRef() const KIT_NOEXCEPT
    {
        if (m_RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
            static_cast<const T *>(this)->selfDestruct();
    }

    mutable std::atomic_int32_t m_RefCount;

    template <typename U> friend class Ref;
};

// To use const, Ref<const T> should be enough
template <typename T> class Ref
{
    KIT_OVERRIDE_NEW_DELETE(Ref<T>, 32)
  public:
    Ref() KIT_NOEXCEPT = default;

    /* COPY-MOVE CTORS */
    explicit(false) Ref(T *p_Ptr) KIT_NOEXCEPT : m_Ptr(p_Ptr)
    {
        increaseRef();
    }
    Ref(const Ref &p_Other) KIT_NOEXCEPT : m_Ptr(p_Other.m_Ptr)
    {
        increaseRef();
    }
    Ref(Ref &&p_Other) noexcept : m_Ptr(p_Other.m_Ptr)
    {
        p_Other.m_Ptr = nullptr;
    }

    /* COPY-MOVE ASSIGNMENTS */
    Ref &operator=(T *p_Ptr) KIT_NOEXCEPT
    {
        if (m_Ptr != p_Ptr)
        {
            decreaseRef();
            m_Ptr = p_Ptr;
            increaseRef();
        }
        return *this;
    }
    Ref &operator=(const Ref &p_Other) KIT_NOEXCEPT
    {
        if (m_Ptr != p_Other.m_Ptr)
        {
            decreaseRef();
            m_Ptr = p_Other.m_Ptr;
            increaseRef();
        }
        return *this;
    }
    Ref &operator=(Ref &&p_Other) noexcept
    {
        if (m_Ptr != p_Other.m_Ptr)
        {
            decreaseRef();
            m_Ptr = p_Other.m_Ptr;

            // I have considered moving this out of the if (and calling increaseRef() on the next line) to avoid the
            // small inconsistency (sometimes moved object gets nuked?), but I dont think it will cause any issues
            p_Other.m_Ptr = nullptr;
        }
        return *this;
    }

    /* COPY-MOVE CTORS TEMPLATED VARIANTS */
    template <typename U> explicit(false) Ref(const Ref<U> &p_Other) KIT_NOEXCEPT : m_Ptr(p_Other.m_Ptr)
    {
        increaseRef();
    }

    template <typename U> explicit(false) Ref(Ref<U> &&p_Other) noexcept : m_Ptr(p_Other.m_Ptr)
    {
        p_Other.m_Ptr = nullptr;
    }

    /* COPY-MOVE ASSIGNMENTS TEMPLATED VARIANTS */
    template <typename U> Ref &operator=(const Ref<U> &p_Other) KIT_NOEXCEPT
    {
        if (m_Ptr != p_Other.m_Ptr)
        {
            decreaseRef();
            m_Ptr = p_Other.m_Ptr;
            increaseRef();
        }
        return *this;
    }

    template <typename U> Ref &operator=(Ref<U> &&p_Other) KIT_NOEXCEPT
    {
        if (m_Ptr != p_Other.m_Ptr)
        {
            decreaseRef();
            m_Ptr = p_Other.m_Ptr;
            p_Other.m_Ptr = nullptr;
            return *this;
        }
    }

    ~Ref() KIT_NOEXCEPT
    {
        decreaseRef();
    }

    T *operator->() const KIT_NOEXCEPT
    {
        return m_Ptr;
    }
    T &operator*() const KIT_NOEXCEPT
    {
        return *m_Ptr;
    }
    explicit(false) operator T *() const KIT_NOEXCEPT
    {
        return m_Ptr;
    }
    explicit(false) operator bool() const KIT_NOEXCEPT
    {
        return m_Ptr != nullptr;
    }

    T *Get() const KIT_NOEXCEPT
    {
        return m_Ptr;
    }

    std::strong_ordering operator<=>(const Ref &p_Other) const KIT_NOEXCEPT = default;

  private:
    void increaseRef() const KIT_NOEXCEPT
    {
        // Must static_cast to RefCounted to access increaseRef
        if (m_Ptr)
            static_cast<const RefCounted<typename T::CountedType> *>(m_Ptr)->increaseRef();
    }
    void decreaseRef() const KIT_NOEXCEPT
    {
        // Must static_cast to RefCounted to access decreaseRef
        if (m_Ptr)
            static_cast<const RefCounted<typename T::CountedType> *>(m_Ptr)->decreaseRef();
    }

    T *m_Ptr = nullptr;
    template <typename U> friend class Ref;
};

template <typename T>
concept RCounted = std::is_base_of_v<RefCounted<typename T::CountedType>, T>;

KIT_NAMESPACE_END

namespace std
{
template <typename T> struct hash<KIT_NAMESPACE_NAME::Ref<T>>
{
    KIT_NAMESPACE_NAME::usz operator()(const KIT_NAMESPACE_NAME::Ref<T> &p_Ref) const KIT_NOEXCEPT
    {
        return hash<T *>()(p_Ref.Get());
    }
};
} // namespace std