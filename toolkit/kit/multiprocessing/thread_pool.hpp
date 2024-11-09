#pragma once

#include "kit/core/concepts.hpp"
#include "kit/multiprocessing/task_manager.hpp"
#include <thread>

namespace KIT
{
// TODO: Implement a lock-free queue
// TODO: Consider adding dependencies

/**
 * @brief A thread pool that manages tasks and executes them in parallel.
 *
 * It is a concrete class that implements the ITaskManager interface.
 *
 * @note The thread pool is not resizable. Once created, the number of threads is fixed. Currently, queue
 * synchronization is achieved by a plain mutex lock, which is not ideal. I have considered implementing a lock-free
 * approach, but I wont do it until the need arises, as those are very complex to get right.
 *
 * @tparam MTX The type of mutex to use for synchronization. It must implement the Mutex concept.
 */
template <Mutex MTX> class ThreadPool final : public ITaskManager
{
  public:
    explicit ThreadPool(usize p_ThreadCount);
    ~ThreadPool() noexcept override;

    /**
     * @brief Submit a task to be executed by the thread pool.
     *
     * The task will be executed as soon as possible.
     *
     * @param p_Task The task to submit.
     */
    void SubmitTask(const Ref<ITask> &p_Task) noexcept override;

    /**
     * @brief Wait for all pending tasks to finish executing.
     *
     */
    void AwaitPendingTasks() const noexcept;

  private:
    DynamicArray<std::thread> m_Threads;
    Deque<Ref<ITask>> m_Queue;

    std::atomic_flag m_TaskReady = ATOMIC_FLAG_INIT;
    std::atomic_flag m_Shutdown = ATOMIC_FLAG_INIT;

    std::atomic<u32> m_TerminatedCount = 0;
    std::atomic<u32> m_PendingCount = 0;

    mutable MTX m_Mutex;
};
} // namespace KIT