#include "core/pch.hpp"
#include "kit/multiprocessing/thread_pool.hpp"

KIT_NAMESPACE_BEGIN

ThreadPool::ThreadPool(const u32 p_ThreadCount) : TaskManager(p_ThreadCount)
{
    m_Threads.reserve(p_ThreadCount);
    const auto worker = [this](const usz p_ThreadIndex) {
        for (;;)
        {
            m_TaskReady.wait(false, std::memory_order_relaxed);
            if (m_Shutdown.test(std::memory_order_relaxed))
                break;

            // This could potentially be lock free by implementing a lock free deque
            std::unique_lock lock(m_Mutex);
            if (m_Queue.empty())
            {
                m_TaskReady.clear(std::memory_order_relaxed);
                continue;
            }

            const Ref<ITask> task = m_Queue.front();
            m_Queue.pop_front();

            lock.unlock();
            (*task)(p_ThreadIndex);
            m_PendingCount.fetch_sub(1, std::memory_order_relaxed);
        }
        m_TerminatedCount.fetch_add(1, std::memory_order_relaxed);
    };
    for (u32 i = 0; i < p_ThreadCount; ++i)
        m_Threads.emplace_back(worker, i);
}

void ThreadPool::AwaitPendingTasks() const KIT_NOEXCEPT
{
    while (m_PendingCount.load(std::memory_order_relaxed) != 0)
        std::this_thread::yield();
}

ThreadPool::~ThreadPool() KIT_NOEXCEPT
{
    // It is up to the user to make sure that all tasks are finished before destroying the thread pool
    m_Shutdown.test_and_set(std::memory_order_relaxed);

    // Aggressively force all threads out
    while (m_PendingCount.load(std::memory_order_relaxed) != 0 ||
           m_TerminatedCount.load(std::memory_order_relaxed) != ThreadCount())
    {
        m_TaskReady.test_and_set(std::memory_order_relaxed);
        m_TaskReady.notify_all();
    }

    for (std::thread &thread : m_Threads)
        thread.join();
    KIT_LOG_WARNING_IF(!m_Queue.empty(), "Destroying thread pool with pending tasks. Executing them serially now...");
    while (!m_Queue.empty())
    {
        const Ref<ITask> task = m_Queue.front();
        m_Queue.pop_front();
        (*task)(0);
    }
}

void ThreadPool::SubmitTask(const Ref<ITask> &p_Task) KIT_NOEXCEPT
{
    m_PendingCount.fetch_add(1, std::memory_order_relaxed);
    {
        std::scoped_lock lock(m_Mutex);
        m_Queue.push_back(p_Task);
    }
    m_TaskReady.test_and_set(std::memory_order_release);
    m_TaskReady.notify_one();
}

KIT_NAMESPACE_END