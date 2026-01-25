#pragma once

#ifndef TKIT_ENABLE_MULTIPROCESSING
#    error                                                                                                             \
        "[TOOLKIT][MULTIPROC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_MULTIPROCESSING"
#endif

#include "tkit/multiprocessing/task_manager.hpp"
#include "tkit/multiprocessing/chase_lev_deque.hpp"
#include "tkit/multiprocessing/mpmc_stack.hpp"
#include "tkit/container/arena_array.hpp"
#include "tkit/multiprocessing/topology.hpp"
#include <thread>

namespace TKit
{
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
 * Only one `ThreadPool` object may exist at any given time. Having more is theoretically possible but may (and will)
 * lead to errors and problems, especially with thread indices.
 *
 */
TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_MSVC_WARNING_IGNORE(4324)
class ThreadPool final : public ITaskManager
{
  public:
    struct alignas(TKIT_CACHE_LINE_SIZE) Worker
    {
        template <typename Callable, typename... Args>
        Worker(ArenaAllocator *allocator, const usize maxTasks, Callable &&callable, Args &&...args)
            : Thread(std::forward<Callable>(callable), std::forward<Args>(args)...), Queue(allocator, maxTasks)
        {
        }
        std::thread Thread;
        ChaseLevDeque<ITask *> Queue;
        MpmcStack<ITask *> Inbox{};
        std::atomic<u32> Epochs{0};
        std::atomic<u32> TaskCount{0}; // Speculative
        std::atomic_flag TerminateSignal = ATOMIC_FLAG_INIT;
    };

    ThreadPool(ArenaAllocator *allocator, usize wokerCount, usize maxTasksPerQueue = 32);
    explicit ThreadPool(usize wokerCount, usize maxTasksPerQueue = 32);
    ~ThreadPool() override;

    /**
     * @brief Submit a task to be executed by the thread pool.
     *
     * The task will be scheduled and executed as soon as possible.
     *
     * @param task The task to submit.
     * @param submissionIndex An optional submission index to potentially speed up submission process when submitting
     * many tasks in a short period of time (which will certainly almost always be the case). It is completely optional
     * and can be ignored. It should always start at 0 when a new batch of tasks is going to be submitted.
     * @return The next submission index that should be fed to the next task submission while in the same batch.
     */
    usize SubmitTask(ITask *task, usize submissionIndex = 0) override;

    /**
     * @brief Block the calling thread until the task has finished executing.
     *
     * The calling thread will not be idle. While waiting, it will attempt to drain other tasks in the thread pool,
     * while also yielding and ensuring not too much processing power is wasted in case no other tasks are available.
     *
     * This method should always be preferred to the `WaitUntilFinished()` task method. The latter will truly wait and
     * may lead to deadlocks if the task it is waiting on submits a task to the waiting thread and requires it to be
     * completed before moving on.
     *
     */
    void WaitUntilFinished(const ITask &task) override;

    static usize GetWorkerIndex()
    {
        return Topology::GetThreadIndex() - 1;
    }

  private:
    void drainTasks(usize workerIndex, usize workers);
    bool trySteal(usize victim);

    ArenaArray<Worker> m_Workers;

    alignas(TKIT_CACHE_LINE_SIZE) std::atomic_flag m_ReadySignal = ATOMIC_FLAG_INIT;
    const Topology::Handle *m_Handle;
};
TKIT_COMPILER_WARNING_IGNORE_POP()
} // namespace TKit
