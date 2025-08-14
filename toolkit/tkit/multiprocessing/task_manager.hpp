#pragma once

#ifndef TKIT_ENABLE_MULTIPROCESSING
#    error                                                                                                             \
        "[TOOLKIT] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_MULTIPROCESSING"
#endif

#include "tkit/multiprocessing/task.hpp"
#include "tkit/container/span.hpp"
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
    virtual void SubmitTask(ITask *p_Task) noexcept = 0;

    /**
     * @brief Submit a group of tasks to be executed by the task manager.
     *
     * It may provide a speedup compared to calling `SubmitTask()` multiple times.
     *
     * @param p_Tasks The tasks to submit.
     */
    virtual void SubmitTasks(Span<ITask *const> p_Tasks) noexcept = 0;

    /**
     * @brief Create a task allocated with a thread-dedicated allocator.
     *
     * The created task must be deallocated by the same thread it was allocated with.
     *
     * @param p_Callable The callable object to execute.
     * @param p_Args Extra arguments to pass to the callable object.
     * @return A new task object.
     */
    template <typename Callable, typename... Args>
    static auto CreateTask(Callable &&p_Callable, Args &&...p_Args) noexcept
        -> Task<std::invoke_result_t<Callable, Args...>> *
    {
        using RType = std::invoke_result_t<Callable, Args...>;
        return Task<RType>::Create(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Destroy a task, deallocating it with the calling thread's dedicated allocator.
     *
     * The calling thread must be the one that allocated the task in the first place.
     *
     * @param p_Task The task to be destroyed.
     */
    template <typename T> static void DestroyTask(Task<T> *p_Task) noexcept
    {
        Task<T>::Destroy(p_Task);
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
        -> Task<std::invoke_result_t<Callable, Args...>> *
    {
        using RType = std::invoke_result_t<Callable, Args...>;
        Task<RType> *task = CreateTask(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...);
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
    static usize GetThreadIndex() noexcept;

  protected:
    static inline thread_local usize s_ThreadIndex = 0;

  private:
    usize m_ThreadCount;
};
} // namespace TKit
