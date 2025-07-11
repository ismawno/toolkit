#include "tkit/core/pch.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include <mutex>

namespace TKit
{
ThreadPool::ThreadPool(const usize p_ThreadCount) : ITaskManager(p_ThreadCount)
{
    const auto worker = [this](const usize p_ThreadIndex) {
        for (;;)
        {
            s_ThreadIndex = p_ThreadIndex;
            m_TaskReady.wait(false, std::memory_order_relaxed);
            if (m_Shutdown.test(std::memory_order_relaxed))
                break;

            Ref<ITask> task;
            { // This could potentially be lock free by implementing a lock free deque
                std::scoped_lock lock(m_Mutex);
                TKIT_PROFILE_MARK_LOCK(m_Mutex);
                if (m_Queue.IsEmpty())
                {
                    m_TaskReady.clear(std::memory_order_relaxed);
                    continue;
                }

                task = m_Queue.GetBack();
                m_Queue.Pop();
            }

            (*task)(p_ThreadIndex);
            m_PendingCount.fetch_sub(1, std::memory_order_release);
        }
        m_TerminatedCount.fetch_add(1, std::memory_order_relaxed);
    };
    for (usize i = 0; i < p_ThreadCount; ++i)
        m_Threads.Append(worker, i + 1);
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
    TKIT_LOG_WARNING_IF(!m_Queue.IsEmpty(),
                        "[TOOLKIT] Destroying thread pool with pending tasks. Executing them serially now...");
    while (!m_Queue.IsEmpty())
    {
        const Ref<ITask> task = m_Queue.GetBack();
        m_Queue.Pop();
        (*task)(0);
    }
}

void ThreadPool::AwaitPendingTasks() const noexcept
{
    // TODO: Consider using _mm_pause() instead of std::this_thread::yield()
    while (m_PendingCount.load(std::memory_order_acquire) != 0)
        std::this_thread::yield();
}

usize ThreadPool::GetThreadIndex() const noexcept
{
    return s_ThreadIndex;
}

void ThreadPool::SubmitTask(const Ref<ITask> &p_Task) noexcept
{
    m_PendingCount.fetch_add(1, std::memory_order_relaxed);
    {
        std::scoped_lock lock(m_Mutex);
        TKIT_PROFILE_MARK_LOCK(m_Mutex);
        m_Queue.Insert(m_Queue.begin(), p_Task);
    }
    m_TaskReady.test_and_set(std::memory_order_release);
    m_TaskReady.notify_one();
}
} // namespace TKit
