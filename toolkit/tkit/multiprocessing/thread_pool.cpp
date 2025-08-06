#include "tkit/core/pch.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include <mutex>
#include <string>
#ifdef TKIT_OS_WINDOWS
#    include <windows.h>
#else
#    include <pthread.h>
#    include <sched.h>
#endif

namespace TKit
{
#ifdef TKIT_OS_WINDOWS
static void SetAffinityAndName(const u32 p_ThreadIndex, const char *p_Name = nullptr) noexcept
{
    const u32 totalCores = std::thread::hardware_concurrency();
    const u32 coreId = p_ThreadIndex % totalCores;

    const DWORD_PTR mask = 1ULL << coreId;
    const HANDLE thread = GetCurrentThread();
    TKIT_ASSERT_NOT_RETURNS(SetThreadAffinityMask(thread, mask), 0);
    if (p_Name)
    {
        SetThreadDescription(thread, name.c_str());
        return;
    }

    std::ostringstream oss;
    oss << "tkit-worker-" << p_ThreadIndex;
    const auto str = oss.str();
    const std::wstring name(str.begin(), str.end());

    SetThreadDescription(thread, name.c_str());
}
#else
static void SetAffinityAndName(const u32 p_ThreadIndex, const char *p_Name = nullptr) noexcept
{
    const u32 totalCores = std::thread::hardware_concurrency();
    const u32 coreId = p_ThreadIndex % totalCores;

    cpu_set_t cpu;
    CPU_ZERO(&cpu);
    CPU_SET(coreId, &cpu);

    const pthread_t current = pthread_self();
    TKIT_ASSERT_RETURNS(pthread_setaffinity_np(current, sizeof(cpu_set_t), &cpu), 0);
    if (p_Name)
    {
        pthread_setname_np(current, p_Name);
        return;
    }

    const std::string name = "tkit-worker-" + std::to_string(p_ThreadIndex);
    pthread_setname_np(current, name.c_str());
}
#endif
ThreadPool::ThreadPool(const usize p_ThreadCount) : ITaskManager(p_ThreadCount)
{
    SetAffinityAndName(0, "tkit-main");
    const auto worker = [this](const usize p_ThreadIndex) {
        SetAffinityAndName(p_ThreadIndex);
        s_ThreadIndex = p_ThreadIndex;
        for (;;)
        {
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
                m_Queue.PopBack();
            }

            (*task)();
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
        m_Queue.PopBack();
        (*task)();
    }
}

void ThreadPool::AwaitPendingTasks() const noexcept
{
    // TODO: Consider using _mm_pause() instead of std::this_thread::yield()
    while (m_PendingCount.load(std::memory_order_acquire) != 0)
        std::this_thread::yield();
}

void ThreadPool::SubmitTask(const Ref<ITask> &p_Task) noexcept
{
    m_PendingCount.fetch_add(1, std::memory_order_relaxed);
    {
        std::scoped_lock lock(m_Mutex);
        TKIT_PROFILE_MARK_LOCK(m_Mutex);
        m_Queue.PushFront(p_Task);
    }
    m_TaskReady.test_and_set(std::memory_order_release);
    m_TaskReady.notify_one();
}
} // namespace TKit
