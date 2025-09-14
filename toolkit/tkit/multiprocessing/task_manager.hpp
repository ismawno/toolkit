#pragma once

#ifndef TKIT_ENABLE_MULTIPROCESSING
#    error                                                                                                             \
        "[TOOLKIT][MULTIPROC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_MULTIPROCESSING"
#endif

#include "tkit/multiprocessing/task.hpp"
#include "tkit/container/span.hpp"
#include <type_traits>

namespace TKit
{
/**
 * @brief A task manager that is responsible for managing tasks and executing them.
 *
 * It is an abstract class that must be implemented by the user to create a custom task system.
 *
 */
class TKIT_API ITaskManager
{
  public:
    ITaskManager(usize p_WorkerCount);
    virtual ~ITaskManager() = default;

    /**
     * @brief Submit a task to be executed by the task manager.
     *
     * The task will be executed as soon as possible.
     *
     * @param p_Task The task to submit.
     */
    virtual void SubmitTask(ITask *p_Task) = 0;

    virtual void WaitUntilFinished(const ITask &p_Task) = 0;

    template <typename T> T WaitForResult(const Task<T> &p_Task)
    {
        WaitUntilFinished(p_Task);
        return p_Task.GetResult();
    }

    /**
     * @brief Submit a group of tasks to be executed by the task manager.
     *
     * It may provide a speedup compared to calling `SubmitTask()` multiple times.
     *
     * @param p_Tasks The tasks to submit.
     */
    virtual void SubmitTasks(Span<ITask *const> p_Tasks) = 0;

    /**
     * @brief Submit a group of tasks to be executed by the task manager.
     *
     * It may provide a speedup compared to calling `SubmitTask()` multiple times.
     *
     * @param p_Tasks The tasks to submit.
     */
    template <typename T> void SubmitTasks(const Span<Task<T> *const> p_Tasks)
    {
        const usize size = sizeof(ITask *) * p_Tasks.GetSize();
        TKIT_MEMORY_STACK_CHECK(size);
        ITask **tasks = static_cast<ITask **>(TKIT_MEMORY_STACK_ALLOCATE(size));

        for (u32 i = 0; i < p_Tasks.GetSize(); ++i)
            tasks[i] = p_Tasks[i];

        SubmitTasks(Span<ITask *const>{tasks, p_Tasks.GetSize()});
        TKIT_MEMORY_STACK_DEALLOCATE(tasks);
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
    usize GetWorkerCount() const;

    /**
     * @brief Ask for the thread index of the current thread.
     *
     * @return The index of the current thread.
     */
    static usize GetThreadIndex();

  protected:
    static inline thread_local usize t_ThreadIndex = 0;

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
class TKIT_API TaskManager final : public ITaskManager
{
  public:
    TaskManager();

    void SubmitTask(ITask *p_Task) override;
    void SubmitTasks(Span<ITask *const> p_Tasks) override;

    void WaitUntilFinished(const ITask &p_Task) override;
};
} // namespace TKit
