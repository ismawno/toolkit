#include "kit/core/pch.hpp"
#include "kit/multiprocessing/thread_pool.hpp"
#include "kit/multiprocessing/spin_lock.hpp"

namespace TKit
{
template <Mutex MTX> ThreadPool<MTX>::ThreadPool(const usize p_ThreadCount) : ITaskManager(p_ThreadCount)
{
    m_Threads.reserve(p_ThreadCount);
    const auto worker = [this](const usize p_ThreadIndex) {
        for (;;)
        {
            m_TaskReady.wait(false, std::memory_order_relaxed);
            if (m_Shutdown.test(std::memory_order_relaxed))
                break;

            Ref<ITask> task;
            { // This could potentially be lock free by implementing a lock free deque
                std::scoped_lock lock(m_Mutex);
                TKIT_PROFILE_MARK_LOCK(m_Mutex);
                if (m_Queue.empty())
                {
                    m_TaskReady.clear(std::memory_order_relaxed);
                    continue;
                }

                task = m_Queue.front();
                m_Queue.pop_front();
            }

            (*task)(p_ThreadIndex);
            m_PendingCount.fetch_sub(1, std::memory_order_relaxed);
        }
        m_TerminatedCount.fetch_add(1, std::memory_order_relaxed);
    };
    for (usize i = 0; i < p_ThreadCount; ++i)
        m_Threads.emplace_back(worker, i);
}

template <Mutex MTX> void ThreadPool<MTX>::AwaitPendingTasks() const noexcept
{
    // TODO: Consider using _mm_pause() instead of std::this_thread::yield()
    while (m_PendingCount.load(std::memory_order_relaxed) != 0)
        std::this_thread::yield();
}

template <Mutex MTX> ThreadPool<MTX>::~ThreadPool() noexcept
{
    // It is up to the user to make sure that all tasks are finished before destroying the thread pool
    m_Shutdown.test_and_set(std::memory_order_relaxed);

    // Aggressively force all threads out
    while (m_PendingCount.load(std::memory_order_relaxed) != 0 ||
           m_TerminatedCount.load(std::memory_order_relaxed) != GetThreadCount())
    {
        m_TaskReady.test_and_set(std::memory_order_relaxed);
        m_TaskReady.notify_all();
    }

    for (std::thread &thread : m_Threads)
        thread.join();
    TKIT_LOG_WARNING_IF(!m_Queue.empty(), "Destroying thread pool with pending tasks. Executing them serially now...");
    while (!m_Queue.empty())
    {
        const Ref<ITask> task = m_Queue.front();
        m_Queue.pop_front();
        (*task)(0);
    }
}

template <Mutex MTX> void ThreadPool<MTX>::SubmitTask(const Ref<ITask> &p_Task) noexcept
{
    m_PendingCount.fetch_add(1, std::memory_order_relaxed);
    {
        std::scoped_lock lock(m_Mutex);
        TKIT_PROFILE_MARK_LOCK(m_Mutex);
        m_Queue.push_back(p_Task);
    }
    m_TaskReady.test_and_set(std::memory_order_release);
    m_TaskReady.notify_one();
}

template class ThreadPool<std::mutex>;
template class ThreadPool<SpinLock>;
} // namespace TKit