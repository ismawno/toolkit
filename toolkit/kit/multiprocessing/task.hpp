#pragma once

#include "kit/memory/block_allocator.hpp"
#include "kit/memory/ptr.hpp"
#include <functional>
#include <future>

namespace KIT
{
// TODO: Align/add padding to 64 bytes to avoid false sharing? Profile first.

/**
 * @brief A simple task interface that allows the user to create tasks that can be executed by any task manager that
 * inherits from ITaskManager.
 *
 * The task is a simple callable object that takes a thread index as an argument.
 *
 */
class KIT_API ITask : public RefCounted<ITask>
{
    // This is commented out because it is wasteful to instance a block allocator for ITask that will never be used.
    // Having the overloads does grant us some asserts in case something goes terribly wrong or user forgets to use
    // their/our overriden new/delete, but it is not worth the cost.
    // KIT_BLOCK_ALLOCATED_CONCURRENT(ITask, 32)
    KIT_NON_COPYABLE(ITask)
  public:
    ITask() noexcept = default;
    virtual ~ITask() noexcept = default;

    virtual void operator()(usize p_ThreadIndex) noexcept = 0;

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
    void NotifyCompleted() noexcept;

  private:
    std::atomic_flag m_Finished = ATOMIC_FLAG_INIT;
};

// The generic and the specialized could be merged by moving the specialized bit to a base class that is empty when T is
// void, however this use case is simple enough to not warrant the extra complexity.

/**
 * @brief A task object that can be used directly by the user to create tasks that return (or not) a value.
 *
 * The task is a simple callable object that takes a thread index as an argument. If the task is a simple routine that
 * does not return anything, use the specialized Task<void> instead.
 *
 * The return type T must be default constructible and copy assignable. Once the task has finished executing, the result
 * will be copied into the task object and can be retrieved by calling the WaitForResult() method.
 *
 * @tparam T The return type of the task.
 */
template <typename T> class Task final : public ITask
{
  public:
    KIT_BLOCK_ALLOCATED_CONCURRENT(Task<T>, 32)
    void operator()(const usize p_ThreadIndex) noexcept override
    {
        m_Result = m_Function(p_ThreadIndex);
        NotifyCompleted();
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

    template <typename Callable, typename... Args>
        requires std::invocable<Callable, Args..., usize>
    explicit Task(Callable &&p_Callable, Args &&...p_Args) noexcept
        : m_Function(std::bind_front(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...))
    {
#ifdef KIT_ENABLE_ASSERTS
        if constexpr (sizeof...(Args) == 0)
        {
            KIT_ERROR("Wrong constructor used for Task<T>");
        }
#endif
    }

    template <typename Callable>
        requires(!std::is_same_v<Callable, Task>)
    explicit Task(Callable &&p_Callable) noexcept : m_Function(std::forward<Callable>(p_Callable))
    {
    }

    std::function<T(usize)> m_Function = nullptr;
    T m_Result{};
};

/**
 * @brief A specialized task object that can be used directly by the user to create tasks that do not return a value.
 *
 * The task is a simple callable object that takes a thread index as an argument.
 *
 */
template <> class KIT_API Task<void> final : public ITask
{
  public:
    KIT_BLOCK_ALLOCATED_CONCURRENT(Task<void>, 32)
    void operator()(usize p_ThreadIndex) noexcept override;

    template <typename Callable, typename... Args>
        requires std::invocable<Callable, Args..., usize>
    explicit Task(Callable &&p_Callable, Args &&...p_Args) noexcept
        : m_Function(std::bind_front(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...))
    {
        if constexpr (sizeof...(Args) == 0)
        {
            KIT_ERROR("Wrong constructor used for Task<void>");
        }
    }

    template <typename Callable>
        requires(!std::is_same_v<Callable, Task>)
    explicit Task(Callable &&p_Callable) noexcept : m_Function(std::forward<Callable>(p_Callable))
    {
    }

  private:
    std::function<void(usize)> m_Function = nullptr;
};
} // namespace KIT