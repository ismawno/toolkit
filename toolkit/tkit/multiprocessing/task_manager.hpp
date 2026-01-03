#pragma once

#ifndef TKIT_ENABLE_MULTIPROCESSING
#    error                                                                                                             \
        "[TOOLKIT][MULTIPROC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_MULTIPROCESSING"
#endif

#include "tkit/multiprocessing/task.hpp"
#include "tkit/utils/alias.hpp"
#include <type_traits>

namespace TKit
{
/**
 * @brief A task manager that is responsible for managing tasks and executing them.
 *
 * It is an abstract class that must be implemented by the user to create a custom task system.
 *
 * @note To be able to use it with an interface that accepts an `ITaskManager` base class, the threads owned by the task
 * manager must have their indices set with `TKit::Topology::SetThreadIndex()`. Failing to do so will result in races.
 *
 */
class ITaskManager
{
  public:
    explicit ITaskManager(usize p_WorkerCount);
    virtual ~ITaskManager() = default;

    /**
     * @brief Submit a task to be executed by the task manager.
     *
     * The task will be executed as soon as possible.
     *
     * @param p_Task The task to submit.
     * @param p_SubmissionIndex An optional submission index to potentially speed up submission process when submitting
     * many tasks in a short period of time (which will certainly almost always be the case). It is completely optional
     * and can be ignored. It should always start at 0 when a new batch of tasks is going to be submitted.
     * @return The next submission index that should be fed to the next task submission while in the same batch.
     */
    virtual usize SubmitTask(ITask *p_Task, usize p_SubmissionIndex = 0) = 0;

    /**
     * @brief Block the calling thread until the task has finished executing.
     *
     * This method should always be preferred to the `WaitUntilFinished()` task method. The latter will blindly wait and
     * may lead to deadlocks if the task it is waiting on submits a task to the waiting thread and requires it to be
     * completed before moving on. However, this task manager implementation may let the waiting thread to complete
     * other tasks in the meantime, avoiding the above issue and make better use of the thread's resources.
     *
     */
    virtual void WaitUntilFinished(const ITask &p_Task) = 0;

    /**
     * @brief Block the calling thread until the task has finished executing and return the task's result.
     *
     * This method should always be preferred to the `WaitForResult()` task method. The latter will blindly wait and
     * may lead to deadlocks if the task it is waiting on submits a task to the waiting thread and requires it to be
     * completed before moving on. However, this task manager implementation may let the waiting thread to complete
     * other tasks in the meantime, avoiding the above issue and make better use of the thread's resources.
     *
     * @return The task result.
     *
     */
    template <typename T> T WaitForResult(const Task<T> &p_Task)
    {
        WaitUntilFinished(p_Task);
        return p_Task.GetResult();
    }

    /**
     * @brief Create a task inferred from the return type of the lambda.
     *
     * @param p_Callable The callable object to execute.
     * @param p_Args Extra arguments to pass to the callable object.
     * @return A new task object.
     */
    template <typename Callable, typename... Args>
    static auto CreateTask(Callable &&p_Callable, Args &&...p_Args) -> Task<std::invoke_result_t<Callable, Args...>>
    {
        using RType = std::invoke_result_t<Callable, Args...>;
        return Task<RType>{std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...};
    }

    /**
     * @brief Get the number of workers that the task manager is using.
     *
     */
    usize GetWorkerCount() const
    {
        return m_WorkerCount;
    }

  private:
    usize m_WorkerCount;
};

/**
 * @brief The simplest task manager implementation.
 *
 * This trivial task manager uses only the main thread and executes all of the submitted tasks sequentially and
 * immediately.
 *
 */
class TaskManager final : public ITaskManager
{
  public:
    TaskManager();

    usize SubmitTask(ITask *p_Task, usize) override;

    void WaitUntilFinished(const ITask &p_Task) override;
};
} // namespace TKit
