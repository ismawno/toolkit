#pragma once

#include "tkit/container/array.hpp"
#include <atomic>
#include <optional>

namespace TKit
{
/**
 * @brief A single-producer multiple-consumer double ended queue.
 *
 * The owner of the queue is the only one allowed to push elements into it or pop them from the back. Any thread may pop
 * from the front concurrently.
 *
 * @tparam T The type of the elements in the array.
 * @tparam Capacity The capacity of the array.
 */
template <typename T, u64 Capacity> class ChaseLevDeque
{
    static_assert((Capacity & (Capacity - 1)) == 0, "[TOOLKIT] Chase Lev Deque capacity must be a multiple of 2");

  public:
    static constexpr u64 Mask = Capacity - 1;

    ChaseLevDeque() noexcept = default;

    /**
     * @brief Push a new element into the back of the queue.
     *
     * The element is constructed in place using the provided arguments.
     * This method can only be accessed by the owner of the queue. Concurrent use will cause undefined behaviour.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    void PushBack(Args &&...p_Args) noexcept
    {
        const u64 back = m_Back.load(std::memory_order_relaxed);
        TKIT_ASSERT(back - m_Front.load(std::memory_order_relaxed) < Capacity, "[TOOLKIT] Queue is full!");

        T *region = get(back);
        *region = std::move(T{std::forward<Args>(p_Args)...});

        m_Back.store(back + 1, std::memory_order_release);
    }

    /**
     * @brief Pop an element from the back of the queue.
     *
     * If the queue is empty or if the owner lost a race, the value will be null.
     * This method can only be accessed by the owner of the queue. Concurrent use will cause undefined behaviour.
     *
     * @reture If succeeded, a copy of the element. Otherwise null.
     */
    constexpr std::optional<T> PopBack() noexcept
    {
        const u64 back = m_Back.fetch_sub(1, std::memory_order_relaxed) - 1;
        u64 front = m_Front.load(std::memory_order_relaxed);

        if (back < front)
        {
            m_Back.store(front, std::memory_order_relaxed);
            return std::nullopt;
        }
        T *region = get(back);
        if (back > front)
        {
            const T value = std::move(*region);
            return value;
        }

        if (!m_Front.compare_exchange_strong(front, front + 1, std::memory_order_release, std::memory_order_acquire))
        {
            m_Back.store(front + 1, std::memory_order_relaxed);
            return std::nullopt;
        }

        const T value = std::move(*region);
        m_Back.store(front + 1, std::memory_order_relaxed);
        return value;
    }

    /**
     * @brief Pop an element from the front of the queue.
     *
     * If the queue is empty or if the consumer lost a race, the value will be null.
     * This method can be accessed concurrently by any thread.
     *
     * @return If succeeded, a copy of the element. Otherwise null.
     */
    constexpr std::optional<T> PopFront() noexcept
    {
        u64 front = m_Front.load(std::memory_order_relaxed);
        const u64 back = m_Back.load(std::memory_order_acquire);

        if (back <= front)
            return std::nullopt;

        T *region = get(front);
        const T value = *region;

        if (!m_Front.compare_exchange_strong(front, front + 1, std::memory_order_release, std::memory_order_relaxed))
            return std::nullopt;

        return value;
    }

  private:
    T *get(const u64 p_Index) noexcept
    {
        return &m_Data[p_Index & Mask];
    }

    alignas(TKIT_CACHE_LINE_SIZE) std::atomic<u64> m_Front{0};
    alignas(TKIT_CACHE_LINE_SIZE) std::atomic<u64> m_Back{0};
    Array<T, Capacity, Container::ArrayTraits<T, u64>> m_Data{};
};
} // namespace TKit
