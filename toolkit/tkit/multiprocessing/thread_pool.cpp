#include "tkit/core/pch.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"

namespace TKit
{
ThreadPool::ThreadPool(const usize p_ThreadCount) : ITaskManager(p_ThreadCount)
{
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

                task = m_Queue.back();
                m_Queue.pop_back();
            }

            (*task)(p_ThreadIndex);
            m_PendingCount.fetch_sub(1, std::memory_order_release);
        }
        m_TerminatedCount.fetch_add(1, std::memory_order_relaxed);
    };
    for (usize i = 0; i < p_ThreadCount; ++i)
        m_Threads.emplace_back(worker, i + 1);
}

void ThreadPool::AwaitPendingTasks() const noexcept
{
    // TODO: Consider using _mm_pause() instead of std::this_thread::yield()
    while (m_PendingCount.load(std::memory_order_acquire) != 0)
        std::this_thread::yield();
}

ThreadPool::~ThreadPool() noexcept
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
    TKIT_LOG_WARNING_IF(!m_Queue.empty(),
                        "[TOOLKIT] Destroying thread pool with pending tasks. Executing them serially now...");
    while (!m_Queue.empty())
    {
        const Ref<ITask> task = m_Queue.back();
        m_Queue.pop_back();
        (*task)(0);
    }
}

void ThreadPool::SubmitTask(const Ref<ITask> &p_Task) noexcept
{
    m_PendingCount.fetch_add(1, std::memory_order_relaxed);
    {
        std::scoped_lock lock(m_Mutex);
        TKIT_PROFILE_MARK_LOCK(m_Mutex);
        m_Queue.insert(m_Queue.begin(), p_Task);
    }
    m_TaskReady.test_and_set(std::memory_order_release);
    m_TaskReady.notify_one();
}
} // namespace TKit