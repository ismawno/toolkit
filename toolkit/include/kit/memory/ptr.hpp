#pragma once

#include "kit/core/logging.hpp"
#include "kit/core/concepts.hpp"
#include "kit/memory/block_allocator.hpp"
#include "kit/core/non_copyable.hpp"
#include <memory>
#include <atomic>

namespace KIT
{
// This is a small, homemade implementation of a scope pointer, which is equivalent to a unique_ptr but designed to work
// only with the new/delete operators. Creating a Ref class was somewhat a bit more justified, as I can design it so
// that the user data always holds the reference count, avoiding possible overhead (using those types in a non-reference
// counting scheme is a bit wasteful and inefficient tho). The Scope implementation may seem unnecessary why
// not just use a unique_ptr and thats it? Well, a Scope pointer is easy enough to implement, and it allows me to have a
// bit more consistency with names conventions/factories (+ its fun). Also, I believe unique_ptr has some extra overhead
// due to the need to be able to handle exceptions, which I dont need because I dont use them at all (you can optimize
// that away with a noexcept, true, but I still like the idea of having a custom implementation)

// All in all, I am aware moving out of my way to reinvent the wheel like this is not a good practice, and making
// assumptions about unique_ptr's possible overhead is not a good idea. But for now, this is not important enough to
// change, and I am happy with the result

/**
 * @brief A scope pointer that manages the lifetime of a pointer. It is equivalent to a unique_ptr, but it is designed
 * to work only with the new/delete operators. It definitely has less features than a unique_ptr, but it is simple
 * enough for me to implement it and make it work with the rest of the library.
 *
 * As with the unique_ptr, the Scope pointer is non-copyable, but it is movable. Once destroyed, the pointer is
 * automatically deleted.
 *
 * @tparam T The type of the pointer.
 */
template <typename T> class Scope
{
    KIT_NON_COPYABLE(Scope)
  public:
    Scope() noexcept = default;
    explicit(false) Scope(T *p_Ptr) noexcept : m_Ptr(p_Ptr)
    {
    }
    Scope(Scope &&p_Other) noexcept : m_Ptr(p_Other.m_Ptr)
    {
        p_Other.m_Ptr = nullptr;
    }
    template <typename U> explicit(false) Scope(Scope<U> &&p_Other) noexcept : m_Ptr(p_Other.m_Ptr)
    {
        p_Other.m_Ptr = nullptr;
    }

    Scope &operator=(Scope &&p_Other) noexcept
    {
        if (m_Ptr != p_Other.m_Ptr)
        {
            Release();
            m_Ptr = p_Other.m_Ptr;
            p_Other.m_Ptr = nullptr;
        }
        return *this;
    }
    template <typename U> Scope &operator=(Scope<U> &&p_Other) noexcept
    {
        if (m_Ptr != p_Other.m_Ptr)
        {
            Release();
            m_Ptr = p_Other.m_Ptr;
            p_Other.m_Ptr = nullptr;
        }
        return *this;
    }

    ~Scope() noexcept
    {
        Release();
    }

    /**
     * @brief Release the pointer, deleting it. The pointer is set to nullptr after this operation.
     *
     */
    void Release() noexcept
    {
        if (!m_Ptr)
            return;
        delete m_Ptr;
        m_Ptr = nullptr;
    }

    T *operator->() const noexcept
    {
        return m_Ptr;
    }
    T &operator*() const noexcept
    {
        return *m_Ptr;
    }

    explicit(false) operator bool() const noexcept
    {
        return m_Ptr != nullptr;
    }

    /**
     * @brief Get the pointer.
     *
     */
    T *Get() const noexcept
    {
        return m_Ptr;
    }

    /**
     * @brief Create a new object of type T. This is a factory method that creates a new Scope object.
     *
     * @param p_Args The arguments to pass to the constructor of T.
     * @return Scope A new Scope object.
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    static Scope Create(Args &&...p_Args) noexcept
    {
        return Scope(new T(std::forward<Args>(p_Args)...));
    }

    std::strong_ordering operator<=>(const Scope &p_Other) const noexcept = default;

  private:
    T *m_Ptr = nullptr;

    template <typename U> friend class Scope;
};

// I really tried to use this concept, but msvc wont fucking let me
// template <typename T, template <typename> typename RC>
// concept RCounted = std::is_base_of_v<RC<typename T::CountedType>, T>;

// This is a small homemade implementation of a reference counter to avoid the shared_ptr's allocations overhead.
// This way, the reference counter is stored in the object itself. Any object that wishes to be reference counted
// should inherit from this class

/**
 * @brief A special base class to prepare a type T to be reference counted, granting it with an atomic counter. As of
 * right now, it does not support stack allocated objects. All objects inheriting from this class should be dynamically
 * allocated in some way with the new/delete operators (which may or may not be overloaded).
 *
 * @tparam T The type of the objetc to be reference counted.
 */
template <typename T> class RefCounted
{
  public:
    using CountedType = T;
    // No initialization of refcount. Refcount adds/removes are handled by Ref
    RefCounted() noexcept = default;

    /* COPY-MOVE OPERATIONS */
    // Refcount should not be copied. When doing *ptr1 = *ptr2, I only want to transfer the user's data
    RefCounted(const RefCounted &) noexcept
    {
    }
    RefCounted(RefCounted &&) noexcept
    {
    }

    RefCounted &operator=(const RefCounted &) noexcept
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
    ~RefCounted() noexcept
    {
        KIT_ASSERT(m_RefCount.load(std::memory_order_relaxed) == 0, "RefCounted object deleted with non-zero refcount");
    }
    KIT_WARNING_IGNORE_POP

    u32 RefCount() const noexcept
    {
        return m_RefCount.load(std::memory_order_relaxed);
    }

  protected:
    // User may want to have control over the objects's destruction, so they can implement this method instead of using
    // the default
    void selfDestruct() const noexcept
    {
        delete static_cast<const T *>(this);
    }

  private:
    void increaseRef() const noexcept
    {
        m_RefCount.fetch_add(1, std::memory_order_relaxed);
    }

    void decreaseRef() const noexcept
    {
        if (m_RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
            static_cast<const T *>(this)->selfDestruct();
    }

    mutable std::atomic_int32_t m_RefCount;

    template <typename U> friend class Ref;
};

// To use const, Ref<const T> should be enough

/**
 * @brief A small homemade implementation of a reference counter to avoid some of the shared_ptr's allocations overhead.
 * The reference counter is stored in the object itself. Any object that wishes to be reference counted should inherit
 * from the RefCounted class.
 *
 * @tparam T The type of the pointer.
 */
template <typename T> class Ref
{
    KIT_BLOCK_ALLOCATED_CONCURRENT(Ref<T>, 32)
  public:
    Ref() noexcept = default;

    /* COPY-MOVE CTORS */
    explicit(false) Ref(T *p_Ptr) noexcept : m_Ptr(p_Ptr)
    {
        increaseRef();
    }
    Ref(const Ref &p_Other) noexcept : m_Ptr(p_Other.m_Ptr)
    {
        increaseRef();
    }
    Ref(Ref &&p_Other) noexcept : m_Ptr(p_Other.m_Ptr)
    {
        p_Other.m_Ptr = nullptr;
    }

    /* COPY-MOVE ASSIGNMENTS */
    Ref &operator=(T *p_Ptr) noexcept
    {
        if (m_Ptr != p_Ptr)
        {
            decreaseRef();
            m_Ptr = p_Ptr;
            increaseRef();
        }
        return *this;
    }
    Ref &operator=(const Ref &p_Other) noexcept
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
    template <typename U> explicit(false) Ref(const Ref<U> &p_Other) noexcept : m_Ptr(p_Other.m_Ptr)
    {
        increaseRef();
    }

    template <typename U> explicit(false) Ref(Ref<U> &&p_Other) noexcept : m_Ptr(p_Other.m_Ptr)
    {
        p_Other.m_Ptr = nullptr;
    }

    /* COPY-MOVE ASSIGNMENTS TEMPLATED VARIANTS */
    template <typename U> Ref &operator=(const Ref<U> &p_Other) noexcept
    {
        if (m_Ptr != p_Other.m_Ptr)
        {
            decreaseRef();
            m_Ptr = p_Other.m_Ptr;
            increaseRef();
        }
        return *this;
    }

    template <typename U> Ref &operator=(Ref<U> &&p_Other) noexcept
    {
        if (m_Ptr != p_Other.m_Ptr)
        {
            decreaseRef();
            m_Ptr = p_Other.m_Ptr;
            p_Other.m_Ptr = nullptr;
        }
        return *this;
    }

    ~Ref() noexcept
    {
        decreaseRef();
    }

    T *operator->() const noexcept
    {
        return m_Ptr;
    }
    T &operator*() const noexcept
    {
        return *m_Ptr;
    }
    explicit(false) operator T *() const noexcept
    {
        return m_Ptr;
    }
    explicit(false) operator bool() const noexcept
    {
        return m_Ptr != nullptr;
    }

    /**
     * @brief Get the pointer.
     *
     */
    T *Get() const noexcept
    {
        return m_Ptr;
    }

    /**
     * @brief Create a new object of type T. This is a factory method that creates a new Ref object.
     *
     * @param p_Args The arguments to pass to the constructor of T.
     * @return Ref A new Ref object.
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    static Ref Create(Args &&...p_Args) noexcept
    {
        return Ref(new T(std::forward<Args>(p_Args)...));
    }

    std::strong_ordering operator<=>(const Ref &p_Other) const noexcept = default;

  private:
    void increaseRef() const noexcept
    {
        // Must static_cast to RefCounted to access increaseRef
        if (m_Ptr)
            static_cast<const RefCounted<typename T::CountedType> *>(m_Ptr)->increaseRef();
    }
    void decreaseRef() const noexcept
    {
        // Must static_cast to RefCounted to access decreaseRef
        if (m_Ptr)
            static_cast<const RefCounted<typename T::CountedType> *>(m_Ptr)->decreaseRef();
    }

    T *m_Ptr = nullptr;
    template <typename U> friend class Ref;
};
} // namespace KIT

template <typename T> struct std::hash<KIT::Ref<T>>
{
    KIT::usize operator()(const KIT::Ref<T> &p_Ref) const noexcept
    {
        return std::hash<T *>()(p_Ref.Get());
    }
};
