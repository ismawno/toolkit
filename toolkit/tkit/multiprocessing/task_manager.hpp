#pragma once

#include "tkit/multiprocessing/task.hpp"
#include <type_traits>

namespace TKit
{
/**
 * @brief A task manager that is responsible for managing tasks and executing them. It is an abstract class that
 * must be implemented by the user to create a custom task system.
 *
 * @note A task may only be submitted again if it has finished execution and its Reset() method has been called.
 * Multiple threads can wait for the same task at the same time as long as none of them resets it immediately after.
 * Doing so may cause other threads to wait until the task is submitted and finished again, which may never happen or
 * may be a nasty bug to track down.
 *
 */
class TKIT_API ITaskManager
{
  public:
    explicit ITaskManager(usize p_ThreadCount) noexcept;
    virtual ~ITaskManager() noexcept = default;

    /**
     * @brief Submit a task to be executed by the task manager.
     *
     * The task will be executed as soon as possible.
     *
     * @param p_Task The task to submit.
     */
    virtual void SubmitTask(const Ref<ITask> &p_Task) noexcept = 0;

    /**
     * @brief Create a new task that can be submitted to the task manager.
     *
     * The task is not submitted automatically.
     *
     * @param p_Callable The callable object to execute.
     * @param p_Args Extra arguments to pass to the callable object.
     * @return A new task object.
     */
    template <typename Callable, typename... Args>
    auto CreateTask(Callable &&p_Callable, Args &&...p_Args) const noexcept
        -> Ref<Task<std::invoke_result_t<Callable, Args..., usize>>>
    {
        using RType = std::invoke_result_t<Callable, Args..., usize>;
        return Ref<Task<RType>>::Create(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Create a new task that can be submitted to the task manager and submit it immediately.
     *
     * @param p_Callable The callable object to execute.
     * @param p_Args Extra arguments to pass to the callable object.
     * @return A new task object.
     */
    template <typename Callable, typename... Args>
    auto CreateAndSubmit(Callable &&p_Callable, Args &&...p_Args) noexcept
        -> Ref<Task<std::invoke_result_t<Callable, Args..., usize>>>
    {
        using RType = std::invoke_result_t<Callable, Args..., usize>;
        const Ref<Task<RType>> task = CreateTask(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...);
        SubmitTask(task);
        return task;
    }

    /**
     * @brief Get the number of threads that the task manager is using.
     *
     */
    usize GetThreadCount() const noexcept;

  private:
    usize m_ThreadCount;
    friend class ITask;
};
} // namespace TKit