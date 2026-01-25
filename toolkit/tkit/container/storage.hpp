#pragma once

#include "tkit/memory/memory.hpp"
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
template <usize Size, usize Alignment = alignof(std::max_align_t)> class RawStorage
{
  public:
    constexpr RawStorage() = default;

    /**
     * @brief Construct a new object of type `T` in the local buffer.
     *
     * Calling `Construct()` on top of an existing object will cause undefined behavior. The object of type `T` needs to
     * fit in the local buffer and have an alignment that is compatible with the local buffer.
     *
     * If your type is trivially constructible, you dont need to call this function.
     *
     * @tparam T The type of the object to create.
     * @param args The arguments to pass to the constructor of `T`.
     * @return A pointer to the newly created object.
     */
    template <typename T, typename... Args> constexpr T *Construct(Args &&...args)
    {
        static_assert(sizeof(T) <= Size, "Object does not fit in the local buffer");
        static_assert(alignof(T) <= Alignment, "Object has incompatible alignment");
        return Memory::Construct(Get<T>(), std::forward<Args>(args)...);
    }

    /**
     * @brief Destroy the object in the local buffer.
     *
     * Calling `Destruct()` on top of an already destroyed object/uninitialized memory, or calling `Destruct()` with a
     * different type `T` will cause undefined behavior.
     *
     * If `T`is trivially destructible, this function will do nothing.
     *
     * This function is declared as const to follow the standard pattern where a pointer to const object can be
     * destroyed.
     *
     * @tparam T The type of the object to destroy.
     */
    template <typename T> constexpr void Destruct() const
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::Destruct(Get<T>());
    }

    /**
     * @brief Get a pointer to the object in the local buffer.
     *
     * Calling `Get()` with a different type `T` will cause undefined behavior (uses reinterpret_cast under the hood).
     *
     * @tparam T The type of the object to get.
     * @return A pointer to the object in the local buffer.
     */
    template <typename T> constexpr const T *Get() const
    {
        return reinterpret_cast<const T *>(m_Data);
    }

    /**
     * @brief Get a pointer to the object in the local buffer.
     *
     * Calling `Get()` with a different type `T` will cause undefined behavior (uses reinterpret_cast under the hood).
     *
     * @tparam T The type of the object to get.
     * @return A pointer to the object in the local buffer.
     */
    template <typename T> constexpr T *Get()
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
 * To avoid a boolean check overhead and grant the user more constrol over the destruction of the object, the `T`'s
 * destructor will not be called automatically when the Storage object goes out of scope.
 *
 * @tparam T The type of object to store.
 */
template <typename T> class Storage
{
  public:
    constexpr Storage() = default;

    constexpr Storage(const Storage &other)
        requires std::copy_constructible<T>
    {
        m_Storage.template Construct<T>(*other.Get());
    }
    constexpr Storage(Storage &&other)
        requires std::move_constructible<T>
    {
        m_Storage.template Construct<T>(std::move(*other.Get()));
    }

    constexpr Storage &operator=(const Storage &other)
        requires std::is_copy_assignable_v<T>
    {
        if (this != &other)
            *Get() = *other.Get();
        return *this;
    }

    constexpr Storage &operator=(Storage &&other)
        requires std::is_move_assignable_v<T>
    {
        if (this != &other)
            *Get() = std::move(*other.Get());
        return *this;
    }

    template <typename... Args>
        requires(!std::same_as<Storage, std::decay_t<Args>> && ...)
    constexpr Storage(Args &&...args)
    {
        m_Storage.template Construct<T>(std::forward<Args>(args)...);
    }

    /**
     * @brief Construct a new object of type `T` in the local buffer.
     *
     * Calling `Construct()` on top of an existing object will cause undefined behavior.
     *
     * @param args The arguments to pass to the constructor of `T`.
     * @return A pointer to the newly created object.
     */
    template <typename... Args> constexpr T *Construct(Args &&...args)
    {
        return m_Storage.template Construct<T>(std::forward<Args>(args)...);
    }

    /**
     * @brief Destruct the object in the local buffer.
     *
     * Calling `Destruct()` on top of an already destroyed object/uninitialized memory, or calling `Destruct()` with a
     * different type `T` will cause undefined behavior.
     *
     * This function is declared as const to follow the standard pattern where a pointer to const object can be
     * destroyed.
     */
    constexpr void Destruct() const
    {
        m_Storage.template Destruct<T>();
    }

    constexpr const T *Get() const
    {
        return m_Storage.template Get<T>();
    }
    constexpr T *Get()
    {
        return m_Storage.template Get<T>();
    }

    constexpr const T *operator->() const
    {
        return Get();
    }
    constexpr T *operator->()
    {
        return Get();
    }

    constexpr const T &operator*() const
    {
        return *Get();
    }
    constexpr T &operator*()
    {
        return *Get();
    }

  private:
    RawStorage<sizeof(T), alignof(T)> m_Storage;
};

}; // namespace TKit
