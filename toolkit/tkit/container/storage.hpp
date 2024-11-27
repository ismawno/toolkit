#pragma once

#include "tkit/core/alias.hpp"

namespace TKit
{
/**
 * @brief A raw storage class mainly used to allow the deferred creation and destruction of objects using a fixed size
 * buffer with no heap allocations.
 *
 * This is useful when having a class with no default constructor (having strict initialization requirements) that is
 * being used in another class for which the construction requirements of the original class may not be met at the time
 * of construction.
 *
 * This class is trivially copyable and movable. Be cautios when using it with complex types that have non-trivial copy
 * or move constructors.
 *
 * @tparam Size The size of the local allocation.
 * @tparam Alignment The alignment of the local allocation, defaults to the alignment of a pointer.
 */
template <usize Size, usize Alignment = alignof(void *)> class RawStorage
{
  public:
    RawStorage() noexcept = default;

    /**
     * @brief Construct a new object of type T in the local buffer.
     *
     * Calling Create on top of an existing object will cause undefined behavior. The object of type T needs to fit in
     * the local buffer and have an alignment that is compatible with the local buffer.
     *
     * @tparam T The type of the object to create.
     * @param p_Args The arguments to pass to the constructor of T.
     * @return T* A pointer to the newly created object.
     */
    template <typename T, typename... Args> T *Create(Args &&...p_Args) noexcept
    {
        static_assert(sizeof(T) <= Size, "Object does not fit in the local buffer");
        static_assert(alignof(T) <= Alignment, "Object has incompatible alignment");
        return ::new (m_Data) T(std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Destroy the object in the local buffer.
     *
     * Calling Destroy on top of an already destroyed object/uninitialized memory, or calling Destroy with a different
     * type T will cause undefined behavior.
     *
     * This function is declared as const to follow the standard pattern where a pointer to const object can be
     * destroyed.
     *
     * @tparam T The type of the object to destroy.
     */
    template <typename T> void Destroy() const noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            const T *ptr = Get<T>();
            ptr->~T();
        }
    }

    /**
     * @brief Get a pointer to the object in the local buffer.
     *
     * Calling Get with a different type T will cause undefined behavior (uses reinterpret_cast under the hood).
     *
     * @tparam T The type of the object to get.
     * @return const T* A pointer to the object in the local buffer.
     */
    template <typename T> const T *Get() const noexcept
    {
        return reinterpret_cast<const T *>(m_Data);
    }

    /**
     * @brief Get a pointer to the object in the local buffer.
     *
     * Calling Get with a different type T will cause undefined behavior (uses reinterpret_cast under the hood).
     *
     * @tparam T The type of the object to get.
     * @return T* A pointer to the object in the local buffer.
     */
    template <typename T> T *Get() noexcept
    {
        return reinterpret_cast<T *>(m_Data);
    }

  private:
    alignas(Alignment) std::byte m_Data[Size];
};

/**
 * @brief A class that wraps a RawStorage object and provides a more user-friendly interface for creating and destroying
 * objects.
 *
 * It is safer to use as it emposes more restrictions on its usage. It will adapt to the size and alignment of
 * the specified type, and the copy and move constructors and assignment operators will be generated based on the type
 * T.
 *
 * To avoid a boolean check overhead and grant the user more constrol over the destruction of the object, the T's
 * destructor will not be called automatically when the Storage object goes out of scope.
 *
 * @tparam T The type of object to store.
 */
template <typename T> class Storage
{
  public:
    Storage() noexcept = default;

    Storage(const Storage &p_Other) noexcept
        requires std::copy_constructible<T>
    {
        m_Storage.template Create<T>(*p_Other.Get());
    }
    Storage(Storage &&p_Other) noexcept
        requires std::move_constructible<T>
    {
        m_Storage.template Create<T>(std::move(*p_Other.Get()));
    }

    Storage &operator=(const Storage &p_Other) noexcept
        requires std::is_copy_assignable_v<T>
    {
        if (this != &p_Other)
            *Get() = *p_Other.Get();
        return *this;
    }

    Storage &operator=(Storage &&p_Other) noexcept
        requires std::is_move_assignable_v<T>
    {
        if (this != &p_Other)
            *Get() = std::move(*p_Other.Get());
        return *this;
    }

    template <typename... Args>
        requires(!std::same_as<Storage, std::decay_t<Args>> && ...)
    Storage(Args &&...p_Args) noexcept
    {
        m_Storage.template Create<T>(std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Construct a new object of type T in the local buffer.
     *
     * Calling Create on top of an existing object will cause undefined behavior.
     *
     * @param p_Args The arguments to pass to the constructor of T.
     * @return T* A pointer to the newly created object.
     */
    template <typename... Args> T *Create(Args &&...p_Args) noexcept
    {
        return m_Storage.template Create<T>(std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Destroy the object in the local buffer.
     *
     * Calling Destroy on top of an already destroyed object/uninitialized memory, or calling Destroy with a different
     * type T will cause undefined behavior.
     *
     * This function is declared as const to follow the standard pattern where a pointer to const object can be
     * destroyed.
     */
    void Destroy() const noexcept
    {
        m_Storage.template Destroy<T>();
    }

    const T *Get() const noexcept
    {
        return m_Storage.template Get<T>();
    }
    T *Get() noexcept
    {
        return m_Storage.template Get<T>();
    }

    const T *operator->() const noexcept
    {
        return Get();
    }
    T *operator->() noexcept
    {
        return Get();
    }

    const T &operator*() const noexcept
    {
        return *Get();
    }
    T &operator*() noexcept
    {
        return *Get();
    }

  private:
    RawStorage<sizeof(T), alignof(T)> m_Storage;
};

}; // namespace TKit