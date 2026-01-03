#pragma once

#ifndef TKIT_ENABLE_MULTIPROCESSING
#    error                                                                                                             \
        "[TOOLKIT][MULTIPROC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_MULTIPROCESSING"
#endif

#include "tkit/preprocessor/system.hpp"
#include <functional>
#include <atomic>

namespace TKit
{
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
class ITask
{
  public:
    constexpr ITask() = default;
    virtual ~ITask() = default;

    virtual void operator()() = 0;

    /**
     * @brief Check if the task has finished executing.
     *
     * @param p_Order The memory order of the operation.
     *
     */
    bool IsFinished(const std::memory_order p_Order = std::memory_order_relaxed) const
    {
        return m_Finished.test(p_Order);
    }

    /**
     * @brief Block the calling thread until the task has finished executing.
     *
     * This method may not be safe to use if the thread calling it belongs to the task manager the task was submitted,
     * as deadlocks may happen under heavy load. Even if called from the main thread, if the task it is waiting on gets
     * stranded, it may cause a deadlock as well. It is recommended to always wait for tasks using the `ITaskManager`
     * method `WaitUntilFinished()` instead of this one.
     *
     */
    void WaitUntilFinished() const;

    /**
     * @brief Reset the task so that it can be submitted again.
     *
     */
    void Reset()
    {
        m_Finished.clear(std::memory_order_relaxed);
    }

  protected:
    /**
     * @brief Notify that the task has finished executing.
     *
     */
    void notifyCompleted();

  protected:
    template <typename Callable, typename... Args>
        requires std::invocable<Callable, Args...>
    static constexpr auto bind(Callable &&p_Callable, Args &&...p_Args)
    {
        if constexpr (sizeof...(Args) == 0)
            return p_Callable;
        else
            return std::bind_front(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...);
    }

  private:
    std::atomic_flag m_Finished = ATOMIC_FLAG_INIT;
};

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
    Task() = default;

    template <typename Callable, typename... Args>
        requires(std::invocable<Callable, Args...> && !std::is_same_v<std::remove_cvref_t<Callable>, Task>)
    constexpr explicit Task(Callable &&p_Callable, Args &&...p_Args)
        : m_Function(bind(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...))
    {
    }

    template <typename Callable>
        requires(!std::is_same_v<std::remove_cvref_t<Callable>, Task>)
    constexpr Task &operator=(Callable &&p_Callable)
    {
        m_Function = std::forward<Callable>(p_Callable);
        return *this;
    }

    template <typename Callable, typename... Args>
        requires std::invocable<Callable, Args...>
    constexpr void Set(Callable &&p_Callable, Args &&...p_Args)
    {
        m_Function = std::bind_front(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...);
    }

    void operator()() override
    {
        m_Result = m_Function();
        notifyCompleted();
    }

    /**
     * @brief Block the calling thread until the task has finished executing and return the result.
     *
     * This method may not be safe to use if the thread calling it belongs to the task manager the task was submitted,
     * as deadlocks may happen under heavy load. It is recommended to always wait for tasks using the `ITaskManager`
     * method `WaitForResult()` instead of this one.
     *
     * @return The result of the task.
     */
    const T &WaitForResult() const
    {
        WaitUntilFinished();
        return m_Result;
    }

    /**
     * @brief Retrieve the stored result value of the task.
     *
     * This method must only be called once the task has been waited for. If the task has not finished executing,
     * calling this method is UB and potentially a race.
     *
     * @return The result of the task.
     */
    const T &GetResult() const
    {
        return m_Result;
    }

    operator bool() const
    {
        return m_Function != nullptr;
    }

  private:
    std::function<T()> m_Function = nullptr;
    T m_Result{};
};

/**
 * @brief A specialized task object that can be used directly by the user to create tasks that do not return a value.
 *
 * The task is a simple callable object that takes a thread index as an argument.
 *
 */
template <> class Task<void> final : public ITask
{
  public:
    constexpr Task() = default;

    template <typename Callable, typename... Args>
        requires(std::invocable<Callable, Args...> && !std::is_same_v<std::remove_cvref_t<Callable>, Task>)
    constexpr explicit Task(Callable &&p_Callable, Args &&...p_Args)
        : m_Function(bind(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...))
    {
    }

    template <typename Callable>
        requires(!std::is_same_v<std::remove_cvref_t<Callable>, Task>)
    constexpr Task &operator=(Callable &&p_Callable)
    {
        m_Function = std::forward<Callable>(p_Callable);
        return *this;
    }

    template <typename Callable, typename... Args>
        requires std::invocable<Callable, Args...>
    constexpr void Set(Callable &&p_Callable, Args &&...p_Args)
    {
        m_Function = std::bind_front(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...);
    }

    void operator()() override;

    operator bool() const
    {
        return m_Function != nullptr;
    }

  private:
    std::function<void()> m_Function = nullptr;
};
} // namespace TKit
