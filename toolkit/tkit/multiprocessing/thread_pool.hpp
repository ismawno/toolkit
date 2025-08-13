#pragma once

#ifndef TKIT_ENABLE_MULTIPROCESSING
#    error                                                                                                             \
        "[TOOLKIT] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_MULTIPROCESSING"
#endif

#include "tkit/multiprocessing/task_manager.hpp"
#include "tkit/profiling/macros.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/container/static_deque.hpp"
#include <thread>
#include <mutex>

#ifndef TKIT_THREAD_POOL_MAX_THREADS
#    define TKIT_THREAD_POOL_MAX_THREADS 16
#endif

#ifndef TKIT_THREAD_POOL_MAX_TASKS
#    define TKIT_THREAD_POOL_MAX_TASKS 32
#endif

#if TKIT_THREAD_POOL_MAX_THREADS < 1
#    error "[TOOLKIT] The maximum threads of a thread pool must be greater than one"
#endif

#if TKIT_THREAD_POOL_MAX_TASKS < 1
#    error "[TOOLKIT] The maximum tasks of a thread pool must be greater than one"
#endif

namespace TKit
{
// TODO: Implement a lock-free queue
// TODO: Consider adding task dependencies

/**
 * @brief A thread pool that manages tasks and executes them in parallel.
 *
 * It is a concrete class that implements the `ITaskManager` interface.
 *
 * @note The thread pool is not resizable. Once created, the number of threads is fixed. Currently, queue
 * synchronization is achieved by a plain mutex lock, which is not ideal. I have considered implementing a lock-free
 * approach, but I wont do it until the need arises, as those are very complex to get right.
 *
 * Take into account all threads this pool uses will be secondary threads. By default, the main thread plays no part in
 * the task execution. Please, bear in mind that the thread index is 1-based, as it treats 0 as the main thread in case
 * you want to partition your tasks in such a way that the main thread does some work. If you do not, simply subtract 1
 *
 */
class TKIT_API ThreadPool final : public ITaskManager
{
  public:
    explicit ThreadPool(usize p_ThreadCount);
    ~ThreadPool() noexcept override;

    void SubmitTask(ITask *p_Task) noexcept override;

    /**
     * @brief Wait for all pending tasks to finish executing.
     *
     */
    void AwaitPendingTasks() const noexcept;

  private:
    StaticArray<std::thread, TKIT_THREAD_POOL_MAX_THREADS> m_Threads;
    StaticDeque<ITask *, TKIT_THREAD_POOL_MAX_TASKS> m_Queue;

    std::atomic_flag m_TaskReady = ATOMIC_FLAG_INIT;
    std::atomic_flag m_Shutdown = ATOMIC_FLAG_INIT;

    std::atomic<u32> m_TerminatedCount = 0;
    std::atomic<u32> m_PendingCount = 0;

    mutable TKIT_PROFILE_DECLARE_MUTEX(std::mutex, m_Mutex);
};
} // namespace TKit
