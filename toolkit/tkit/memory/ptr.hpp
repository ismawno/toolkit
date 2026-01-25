#pragma once

#include "tkit/utils/debug.hpp"
#include "tkit/utils/non_copyable.hpp"
#include <atomic>

namespace TKit
{
/**
 * @brief A special base class to prepare a type `T` to be reference counted, granting it with an atomic counter.
 *
 * As of right now, it does not support stack allocated objects. All objects inheriting from this class should be
 * dynamically allocated in some way with the new/delete operators (which may or may not be overloaded).
 *
 * @tparam T The type of the object to be reference counted.
 */
template <typename T> class RefCounted
{
  public:
    using CountedType = T;
    // No initialization of refcount. Refcount adds/removes are handled by Ref
    RefCounted() = default;

    /* COPY-MOVE OPERATIONS */
    // Refcount should not be copied. When doing *ptr1 = *ptr2, I only want to transfer the user's data
    RefCounted(const RefCounted &)
    {
    }
    RefCounted(RefCounted &&)
    {
    }

    RefCounted &operator=(const RefCounted &)
    {
        return *this;
    }
    RefCounted &operator=(RefCounted &&)
    {
        return *this;
    }

    TKIT_COMPILER_WARNING_IGNORE_PUSH()
    TKIT_CLANG_WARNING_IGNORE("-Wexceptions")
    TKIT_GCC_WARNING_IGNORE("-Wterminate")
    TKIT_MSVC_WARNING_IGNORE(4297)
    ~RefCounted()
    {
        TKIT_ASSERT(m_RefCount.load(std::memory_order_relaxed) == 0,
                    "[TOOLKIT] RefCounted object deleted with non-zero refcount: {}",
                    m_RefCount.load(std::memory_order_relaxed));
    }
    TKIT_COMPILER_WARNING_IGNORE_POP()

    u32 RefCount() const
    {
        return m_RefCount.load(std::memory_order_relaxed);
    }

  protected:
    // User may want to have control over the objects's destruction, so they can implement this method instead of using
    // the default
    void selfDestruct() const
    {
        delete static_cast<const T *>(this);
    }

  private:
    void increaseRef() const
    {
        m_RefCount.fetch_add(1, std::memory_order_relaxed);
    }

    void decreaseRef() const
    {
        if (m_RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
            static_cast<const T *>(this)->selfDestruct();
    }

    mutable std::atomic_uint32_t m_RefCount;

    template <typename U> friend class Ref;
};

// To use const, Ref<const T> should be enough

/**
 * @brief A small homemade implementation of a reference counter to avoid some of the shared_ptr's allocations overhead.
 *
 * The reference counter is stored in the object itself. Any object that wishes to be reference counted should inherit
 * from the RefCounted class.
 *
 * @tparam T The type of the pointer.
 */
template <typename T> class Ref
{
  public:
    Ref() = default;

    /* COPY-MOVE CTORS */
    Ref(T *ptr) : m_Ptr(ptr)
    {
        increaseRef();
    }
    Ref(const Ref &other) : m_Ptr(other.m_Ptr)
    {
        increaseRef();
    }
    Ref(Ref &&other) : m_Ptr(other.m_Ptr)
    {
        other.m_Ptr = nullptr;
    }

    /* COPY-MOVE ASSIGNMENTS */
    Ref &operator=(T *ptr)
    {
        if (m_Ptr != ptr)
        {
            decreaseRef();
            m_Ptr = ptr;
            increaseRef();
        }
        return *this;
    }
    Ref &operator=(const Ref &other)
    {
        if (m_Ptr != other.m_Ptr)
        {
            decreaseRef();
            m_Ptr = other.m_Ptr;
            increaseRef();
        }
        return *this;
    }
    Ref &operator=(Ref &&other)
    {
        if (m_Ptr != other.m_Ptr)
        {
            decreaseRef();
            m_Ptr = other.m_Ptr;

            // I have considered moving this out of the if (and calling increaseRef() on the next line) to avoid the
            // small inconsistency (sometimes moved object gets nuked?), but I dont think it will cause any issues
            other.m_Ptr = nullptr;
        }
        return *this;
    }

    /* COPY-MOVE CTORS TEMPLATED VARIANTS */
    template <typename U> Ref(const Ref<U> &other) : m_Ptr(other.m_Ptr)
    {
        increaseRef();
    }

    template <typename U> Ref(Ref<U> &&other) : m_Ptr(other.m_Ptr)
    {
        other.m_Ptr = nullptr;
    }

    /* COPY-MOVE ASSIGNMENTS TEMPLATED VARIANTS */
    template <typename U> Ref &operator=(const Ref<U> &other)
    {
        if (m_Ptr != other.m_Ptr)
        {
            decreaseRef();
            m_Ptr = other.m_Ptr;
            increaseRef();
        }
        return *this;
    }

    template <typename U> Ref &operator=(Ref<U> &&other)
    {
        if (m_Ptr != other.m_Ptr)
        {
            decreaseRef();
            m_Ptr = other.m_Ptr;
            other.m_Ptr = nullptr;
        }
        return *this;
    }

    ~Ref()
    {
        decreaseRef();
    }

    T *operator->() const
    {
        return m_Ptr;
    }
    T &operator*() const
    {
        return *m_Ptr;
    }
    operator T *() const
    {
        return m_Ptr;
    }
    operator bool() const
    {
        return m_Ptr != nullptr;
    }

    /**
     * @brief Get the pointer.
     *
     */
    T *Get() const
    {
        return m_Ptr;
    }

    /**
     * @brief Create a new object of type `T`.
     *
     * This is a factory method that creates a new `Ref<T>` object.
     *
     * @param args The arguments to pass to the constructor of `T`.
     * @return A new `Ref<T>` object.
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    static Ref Create(Args &&...args)
    {
        return Ref(new T(std::forward<Args>(args)...));
    }

    std::strong_ordering operator<=>(const Ref &other) const = default;

  private:
    void increaseRef() const
    {
        // Must static_cast to RefCounted to access increaseRef
        if (m_Ptr)
            static_cast<const RefCounted<typename T::CountedType> *>(m_Ptr)->increaseRef();
    }
    void decreaseRef() const
    {
        // Must static_cast to RefCounted to access decreaseRef
        if (m_Ptr)
            static_cast<const RefCounted<typename T::CountedType> *>(m_Ptr)->decreaseRef();
    }

    T *m_Ptr = nullptr;
    template <typename U> friend class Ref;
};

/**
 * @brief A scope pointer that manages the lifetime of a pointer.
 *
 * It is equivalent to a `unique_ptr`, but it is designed to work only with the new/delete operators. It definitely has
 * less features than a `unique_ptr`, but it is simple enough for me to implement it and make it work with the rest of
 * the library.
 *
 * As with the `unique_ptr`, the Scope pointer is non-copyable, but it is movable. Once destroyed, the pointer is
 * automatically deleted.
 *
 * @tparam T The type of the pointer.
 */
template <typename T> class Scope
{
    TKIT_NON_COPYABLE(Scope)
  public:
    Scope() = default;
    Scope(T *ptr) : m_Ptr(ptr)
    {
    }
    Scope(Scope &&other) : m_Ptr(other.m_Ptr)
    {
        other.m_Ptr = nullptr;
    }
    template <typename U> Scope(Scope<U> &&other) : m_Ptr(other.m_Ptr)
    {
        other.m_Ptr = nullptr;
    }

    Scope &operator=(Scope &&other)
    {
        if (m_Ptr != other.m_Ptr)
            Reset(other.Release());
        return *this;
    }
    template <typename U> Scope &operator=(Scope<U> &&other)
    {
        if (m_Ptr != other.m_Ptr)
            Reset(other.Release());

        return *this;
    }

    ~Scope()
    {
        Reset();
    }

    /**
     * @brief Reset the pointer, deleting it and replacing it by the provided one (which can be nullptr).
     *
     * @param ptr The new pointer to manage.
     */
    void Reset(T *ptr = nullptr)
    {
        delete m_Ptr;
        m_Ptr = ptr;
    }

    /**
     * @brief Release the pointer, returning it and setting the internal pointer to nullptr, which gives up ownership.
     *
     * The caller is now responsible for deleting the pointer.
     *
     * @return The released pointer.
     */
    T *Release()
    {
        T *ptr = m_Ptr;
        m_Ptr = nullptr;
        return ptr;
    }

    /**
     * @brief Transfer the ownership of the pointer to a Ref object, which will manage the lifetime of the pointer.
     *
     * @return A new `Ref<T>` object.
     */
    Ref<T> AsRef()
    {
        return Ref<T>(Release());
    }

    operator Ref<T>()
    {
        return AsRef();
    }

    T *operator->() const
    {
        return m_Ptr;
    }
    T &operator*() const
    {
        return *m_Ptr;
    }

    operator bool() const
    {
        return m_Ptr != nullptr;
    }

    /**
     * @brief Get the pointer.
     *
     */
    T *Get() const
    {
        return m_Ptr;
    }

    /**
     * @brief Create a new object of type `T`.
     *
     * This is a factory method that creates a new Scope object.
     *
     * @param args The arguments to pass to the constructor of `T`.
     * @return A new `Scope<T>` object.
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    static Scope Create(Args &&...args)
    {
        return Scope(new T(std::forward<Args>(args)...));
    }

    std::strong_ordering operator<=>(const Scope &other) const = default;

  private:
    T *m_Ptr = nullptr;

    template <typename U> friend class Scope;
};
} // namespace TKit

template <typename T> struct std::hash<TKit::Ref<T>>
{
    std::size_t operator()(const TKit::Ref<T> &ref) const
    {
        return std::hash<T *>()(ref.Get());
    }
};

template <typename T> struct std::hash<TKit::Scope<T>>
{
    std::size_t operator()(const TKit::Scope<T> &scope) const
    {
        return std::hash<T *>()(scope.Get());
    }
};
