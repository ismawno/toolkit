#pragma once

#include "tkit/utils/non_copyable.hpp"
#include "tkit/container/fixed_array.hpp"
#include "tkit/utils/bit.hpp"
#include "tkit/utils/optional.hpp"
#include <atomic>

namespace TKit
{
/**
 * @brief A single-producer multiple-consumer double ended queue.
 *
 * The owner of the queue is the only one allowed to push elements into it or pop them from the back. Any thread may pop
 * from the front concurrently.
 *
 * It is best used when `T` is lightweight and trivial, so that its atomic counterpart is lock-less.
 *
 * @tparam T The type of the elements in the deque.
 */
template <typename T, usize N> class ChaseLevDeque
{
    static_assert(IsPowerOfTwo(N), "[TOOLKIT][CHASE-LEV] Chase Lev Deque capacity must be a power of 2");

    TKIT_NON_COPYABLE(ChaseLevDeque)
    static constexpr u64 s_Mask = N - 1;

  public:
    using ValueType = T;
    ChaseLevDeque() = default;
    /**
     * @brief Push a new element into the back of the queue.
     *
     * The element is constructed in place using the provided arguments.
     * This method can only be accessed by the owner of the queue. Concurrent use will cause undefined behaviour.
     *
     * @param args The arguments to pass to the constructor of `T`.
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    void PushBack(Args &&...args)
    {
        const u64 back = m_Back.load(std::memory_order_relaxed);
        TKIT_ASSERT(back - m_Front.load(std::memory_order_relaxed) < N,
                    "[TOOLKIT][CHASE-LEV] Cannot PushBack(). Queue is at capacity ({})", N);

        store(back, std::move(T{std::forward<Args>(args)...}));

        std::atomic_thread_fence(std::memory_order_release);
        m_Back.store(back + 1, std::memory_order_relaxed);
    }

    /**
     * @brief Pop an element from the back of the queue.
     *
     * If the queue is empty or if the owner lost a race, the value will be null.
     * This method can only be accessed by the owner of the queue. Concurrent use will cause undefined behaviour.
     *
     * @reture If succeeded, a copy of the element. Otherwise null.
     */
    constexpr Optional<T> PopBack()
    {
        const u64 back = m_Back.fetch_sub(1, std::memory_order_relaxed) - 1;
        std::atomic_thread_fence(std::memory_order_seq_cst);

        u64 front = m_Front.load(std::memory_order_relaxed);

        if (back < front)
        {
            m_Back.store(front, std::memory_order_relaxed);
            return Optional<T>::None();
        }
        if (back > front)
            return load(back);

        if (!m_Front.compare_exchange_strong(front, front + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
        {
            m_Back.store(back + 1, std::memory_order_relaxed);
            return Optional<T>::None();
        }

        return load(back);
    }

    /**
     * @brief Pop an element from the front of the queue.
     *
     * If the queue is empty or if the consumer lost a race, the value will be null.
     * This method can be accessed concurrently by any thread.
     *
     * @return If succeeded, a copy of the element. Otherwise null.
     */
    constexpr Optional<T> PopFront()
    {
        u64 front = m_Front.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        const u64 back = m_Back.load(std::memory_order_acquire);

        if (back <= front)
            return Optional<T>::None();

        const T value = load(front);
        if (!m_Front.compare_exchange_strong(front, front + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
            return Optional<T>::None();

        return value;
    }

  private:
    T load(const u64 index) const
    {
        return m_Data[usize(index & s_Mask)].load(std::memory_order_relaxed);
    }
    void store(const u64 index, T &&element)
    {
        m_Data[usize(index & s_Mask)].store(std::move(element), std::memory_order_relaxed);
    }

    alignas(TKIT_CACHE_LINE_SIZE) std::atomic<u64> m_Front{1};
    alignas(TKIT_CACHE_LINE_SIZE) std::atomic<u64> m_Back{1};
    alignas(TKIT_CACHE_LINE_SIZE) FixedArray<std::atomic<T>, N> m_Data{};
};
} // namespace TKit
