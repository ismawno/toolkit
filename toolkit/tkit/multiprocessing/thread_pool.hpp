#pragma once

#include "tkit/core/concepts.hpp"
#include "tkit/multiprocessing/task_manager.hpp"
#include "tkit/profiling/macros.hpp"
#include "tkit/container/static_array.hpp"
#include <thread>

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
class ThreadPool final : public ITaskManager
{
  public:
    explicit ThreadPool(usize p_ThreadCount);
    ~ThreadPool() noexcept override;

    void SubmitTask(const Ref<ITask> &p_Task) noexcept override;

    /**
     * @brief Wait for all pending tasks to finish executing.
     *
     */
    void AwaitPendingTasks() const noexcept;

  private:
    StaticArray16<std::thread> m_Threads;
    StaticArray32<Ref<ITask>> m_Queue;

    std::atomic_flag m_TaskReady = ATOMIC_FLAG_INIT;
    std::atomic_flag m_Shutdown = ATOMIC_FLAG_INIT;

    std::atomic<u32> m_TerminatedCount = 0;
    std::atomic<u32> m_PendingCount = 0;

    mutable TKIT_PROFILE_DECLARE_MUTEX(std::mutex, m_Mutex);
};
} // namespace TKit