#pragma once

#ifndef TKIT_ENABLE_MULTIPROCESSING
#    error                                                                                                             \
        "[TOOLKIT][MULTIPROC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_MULTIPROCESSING"
#endif

#include "tkit/multiprocessing/task_manager.hpp"
#include "tkit/multiprocessing/chase_lev_deque.hpp"
#include "tkit/multiprocessing/mpmc_stack.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/multiprocessing/topology.hpp"
#include <thread>

#ifndef TKIT_THREAD_POOL_MAX_WORKERS
#    define TKIT_THREAD_POOL_MAX_WORKERS 16
#endif

#ifndef TKIT_THREAD_POOL_MAX_TASKS
#    define TKIT_THREAD_POOL_MAX_TASKS 32
#endif

#if TKIT_THREAD_POOL_MAX_WORKERS < 1
#    error "[TOOLKIT][MULTIPROC] The maximum workers of a thread pool must be greater than one"
#endif

#if TKIT_THREAD_POOL_MAX_TASKS < 1
#    error "[TOOLKIT][MULTIPROC] The maximum tasks of a thread pool must be greater than one"
#endif

namespace TKit
{
// TODO: Implement a lock-free queue
// TODO: Consider adding task dependencies

/**
 * @brief A thread pool that manages tasks and executes them in parallel.
 *
 * It is an implementation of the `ITaskManager` interface.
 *
 * Multiple instances of this thread pool should be possible, but it is not tested. In case many instances exist, it is
 * important that threads do not interact with thread pools they dont belong to.
 *
 * All threads this pool uses will be secondary worker threads. By default, the main thread plays no part in
 * the task execution. Please, bear in mind that the thread index is 1-based, as it treats 0 as the main thread in case
 * you want to partition your tasks in such a way that the main thread does some work. If you do not, simply subtract 1
 * when using the thread index.
 *
 */
class TKIT_API ThreadPool final : public ITaskManager
{
  public:
    struct alignas(TKIT_CACHE_LINE_SIZE) Worker
    {
        template <typename Callable, typename... Args>
        Worker(Callable &&p_Callable, Args &&...p_Args)
            : Thread(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...)
        {
        }

        std::thread Thread;
        ChaseLevDeque<ITask *, TKIT_THREAD_POOL_MAX_TASKS> Queue{};
        MpmcStack<ITask *> Inbox{};
        std::atomic<u64> Epochs{0};
        std::atomic<u32> TaskCount{0}; // Speculative
        std::atomic_flag TerminateSignal = ATOMIC_FLAG_INIT;
        std::atomic_flag TerminateConfirmation = ATOMIC_FLAG_INIT;
    };

    explicit ThreadPool(usize p_ThreadCount);
    ~ThreadPool() noexcept override;

    void SubmitTask(ITask *p_Task) noexcept override;
    void SubmitTasks(Span<ITask *const> p_Tasks) noexcept override;

    void WaitUntilFinished(ITask *p_Task) noexcept override;

    using ITaskManager::SubmitTasks;

    static usize GetWorkerIndex() noexcept;

  private:
    void drainTasks(u32 p_WorkerIndex, u32 p_Workers) noexcept;

    StaticArray<Worker, TKIT_THREAD_POOL_MAX_WORKERS> m_Workers;
    const Topology::Handle *m_Handle;
};
} // namespace TKit
