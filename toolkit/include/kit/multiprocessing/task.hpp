#pragma once

#include "kit/memory/block_allocator.hpp"
#include "kit/memory/ptr.hpp"
#include <functional>
#include <future>

namespace KIT
{
class TaskManager;

// TODO: Align/add padding to 64 bytes to avoid false sharing? Profile first.
class KIT_API ITask : public RefCounted<ITask>
{
    // This is commented out because it is wasteful to instance a block allocator for ITask that will never be used.
    // Having the overloads does grant us some asserts in case something goes terribly wrong or user forgets to use
    // their/our overriden new/delete, but it is not worth the cost.
    // KIT_OVERRIDE_NEW_DELETE(ITask, 32)
    KIT_NON_COPYABLE(ITask)
  public:
    struct Range
    {
        usize Begin;
        usize End;
    };

    virtual ~ITask() noexcept = default;

    virtual void operator()(usize p_ThreadIndex) noexcept = 0;

    bool Valid() const noexcept;
    bool Finished() const noexcept;
    void WaitUntilFinished() const noexcept;

  protected:
    ITask() noexcept = default;

    void NotifyCompleted() noexcept;

  private:
    TaskManager *m_Manager = nullptr;
    std::atomic_flag m_Finished = ATOMIC_FLAG_INIT;

    friend class TaskManager;
};

// The generic and the specialized could be merged by moving the specialized bit to a base class that is empty when T is
// void, however this use case is simple enough to not warrant the extra complexity.
template <typename T> class Task final : public ITask
{
    KIT_OVERRIDE_NEW_DELETE(Task<T>, 32)
  public:
    void operator()(const usize p_ThreadIndex) noexcept override
    {
        m_Result = m_Function(p_ThreadIndex);
        NotifyCompleted();
    }

    const T &WaitForResult() const noexcept
    {
        WaitUntilFinished();
        return m_Result;
    }

  private:
    template <typename Callable, typename... Args>
        requires(!std::is_same_v<Callable, Task>)
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

    friend class TaskManager;
};

template <> class KIT_API Task<void> final : public ITask
{
    KIT_OVERRIDE_NEW_DELETE(Task<void>, 32)
  public:
    void operator()(usize p_ThreadIndex) noexcept override;

    template <typename Callable, typename... Args>
        requires(!std::is_same_v<Callable, Task>)
    explicit Task(Callable &&p_Callable, Args &&...p_Args) noexcept
        : m_Function(std::bind_front(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...))
    {
        KIT_ASSERT(sizeof...(Args) > 0, "Wrong constructor used for Task<void>");
    }

    template <typename Callable>
        requires(!std::is_same_v<Callable, Task>)
    explicit Task(Callable &&p_Callable) noexcept : m_Function(std::forward<Callable>(p_Callable))
    {
    }

  private:
    std::function<void(usize)> m_Function = nullptr;
    friend class TaskManager;
};
} // namespace KIT