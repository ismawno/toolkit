#pragma once

#ifndef TKIT_ENABLE_MULTIPROCESSING
#    error                                                                                                             \
        "[TOOLKIT][MULTIPROC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_MULTIPROCESSING"
#endif

#include "tkit/memory/block_allocator.hpp"
#include <functional>
#include <atomic>

#ifndef TKIT_TASK_ALLOCATOR_CAPACITY
#    define TKIT_TASK_ALLOCATOR_CAPACITY 1024
#endif

namespace TKit
{
// TODO: Align/add padding to 64 bytes to avoid false sharing? Profile first.

/**
 * @brief A simple task interface that allows the user to create tasks that can be executed by any task manager that
 * inherits from `ITaskManager`.
 *
 * The task is a simple callable object that takes a thread index as an argument. To dynamically allocate tasks, you may
 * use the `Create()`/`Destroy()` methods, which will delegate allocation to a specific per-thread allocator. If using
 * this allocation strategy, tasks must be destroyed by the exact same thread that created them. Otherwise, you may
 * allocate tasks on the stack or with the `new` and `delete` operators.
 *
 * @note A task may only be submitted again if it has finished execution and its `Reset()` method has been called.
 * Multiple threads can wait for the same task at the same time as long as none of them resets it immediately after.
 * Doing so may cause other threads to wait until the task is submitted and finished again, which may never happen or
 * may be a nasty bug to track down.
 *
 */
class TKIT_API ITask
{
    // Having the overloads does grant us some asserts in case something goes terribly wrong or user forgets to use
    // their/our overriden new/delete, but it is not worth the cost.
    TKIT_NON_COPYABLE(ITask)
  public:
    ITask() noexcept = default;
    virtual ~ITask() noexcept = default;

    virtual void operator()() noexcept = 0;

    /**
     * @brief Check if the task has finished executing.
     *
     */
    bool IsFinished() const noexcept;

    /**
     * @brief Block the calling thread until the task has finished executing.
     *
     */
    void WaitUntilFinished() const noexcept;

    /**
     * @brief Reset the task so that it can be submitted again.
     *
     */
    void Reset() noexcept;

  protected:
    /**
     * @brief Notify that the task has finished executing.
     *
     */
    void notifyCompleted() noexcept;

  private:
    std::atomic_flag m_Finished = ATOMIC_FLAG_INIT;
};

// The generic and the specialized could be merged by moving the specialized bit to a base class that is empty when T is
// void, however this use case is simple enough to not warrant the extra complexity.

/**
 * @brief A task object that can be used directly by the user to create tasks that return (or not) a value.
 *
 * The task is a simple callable object that takes a thread index as an argument. If the task is a simple routine that
 * does not return anything, use the specialized `Task<void>` instead.
 *
 * The return type `T` must be default constructible and copy assignable. Once the task has finished executing, the
 * result will be copied into the task object and can be retrieved by calling the `WaitForResult()` method.
 *
 * @tparam T The return type of the task.
 */
template <typename T = void> class Task final : public ITask
{
  public:
    void operator()() noexcept override
    {
        m_Result = m_Function();
        notifyCompleted();
    }

    /**
     * @brief Block the calling thread until the task has finished executing and return the result.
     *
     * @return The result of the task.
     */
    const T &WaitForResult() const noexcept
    {
        WaitUntilFinished();
        return m_Result;
    }

    /**
     * @brief Create a task allocated with a thread-dedicated allocator.
     *
     * The created task must be deallocated by the same thread it was allocated with.
     *
     * @return A task pointer.
     */
    template <typename Callable, typename... Args>
        requires std::invocable<Callable, Args...>
    static Task *Create(Callable &&p_Callable, Args &&...p_Args) noexcept
    {
        return s_Allocator.Create<Task>(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...);
    };

    static void Destroy(Task *p_Task) noexcept
    {
        s_Allocator.Destroy(p_Task);
    }

    /**
     * @brief Destroy a task, deallocating it with the calling thread's dedicated allocator.
     *
     * The calling thread must be the one that allocated the task in the first place.
     *
     */
    template <typename Callable, typename... Args>
        requires std::invocable<Callable, Args...>
    explicit Task(Callable &&p_Callable, Args &&...p_Args) noexcept
        : m_Function(std::bind_front(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...))
    {
#ifdef TKIT_ENABLE_ASSERTS
        if constexpr (sizeof...(Args) == 0)
        {
            TKIT_ERROR("Wrong constructor used for Task<T>");
        }
#endif
    }

    template <typename Callable>
        requires(!std::is_same_v<Callable, Task>)
    explicit Task(Callable &&p_Callable) noexcept : m_Function(std::forward<Callable>(p_Callable))
    {
    }

  private:
    std::function<T()> m_Function = nullptr;
    T m_Result{};
    static inline thread_local BlockAllocator s_Allocator =
        BlockAllocator::CreateFromType<Task>(TKIT_TASK_ALLOCATOR_CAPACITY);
};

/**
 * @brief A specialized task object that can be used directly by the user to create tasks that do not return a value.
 *
 * The task is a simple callable object that takes a thread index as an argument.
 *
 */
template <> class TKIT_API Task<void> final : public ITask
{
  public:
    void operator()() noexcept override;

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
        requires std::invocable<Callable, Args...>
    static Task *Create(Callable &&p_Callable, Args &&...p_Args) noexcept
    {
        return s_Allocator.Create<Task>(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...);
    };

    /**
     * @brief Destroy a task, deallocating it with the calling thread's dedicated allocator.
     *
     * The calling thread must be the one that allocated the task in the first place.
     *
     * @param p_Task The task to be destroyed.
     */
    static void Destroy(Task *p_Task) noexcept
    {
        s_Allocator.Destroy(p_Task);
    }

    template <typename Callable, typename... Args>
        requires std::invocable<Callable, Args...>
    explicit Task(Callable &&p_Callable, Args &&...p_Args) noexcept
        : m_Function(std::bind_front(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...))
    {
        if constexpr (sizeof...(Args) == 0)
        {
            TKIT_ERROR("Wrong constructor used for Task<void>");
        }
    }

    template <typename Callable>
        requires(!std::is_same_v<Callable, Task>)
    explicit Task(Callable &&p_Callable) noexcept : m_Function(std::forward<Callable>(p_Callable))
    {
    }

    static BlockAllocator &getAllocator() noexcept;

  private:
    std::function<void()> m_Function = nullptr;
    static inline thread_local BlockAllocator s_Allocator =
        BlockAllocator::CreateFromType<Task>(TKIT_TASK_ALLOCATOR_CAPACITY);
};
} // namespace TKit
