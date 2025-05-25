#pragma once

#ifndef TKIT_ENABLE_MULTIPROCESSING
#    error                                                                                                             \
        "[TOOLKIT] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_MULTIPROCESSING"
#endif

#include "tkit/multiprocessing/task.hpp"
#include <type_traits>

namespace TKit
{
/**
 * @brief A task manager that is responsible for managing tasks and executing them. It is an abstract class that
 * must be implemented by the user to create a custom task system.
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

    /**
     * @brief Ask for the thread index of the current thread.
     *
     * @return The index of the current thread.
     */
    virtual usize GetThreadIndex() const noexcept;

  private:
    usize m_ThreadCount;
};
} // namespace TKit
