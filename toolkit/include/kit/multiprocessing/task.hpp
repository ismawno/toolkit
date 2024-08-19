#pragma once

#include "kit/memory/block_allocator.hpp"
#include "kit/memory/ptr.hpp"
#include <functional>
#include <future>

KIT_NAMESPACE_BEGIN
class TaskManager;

// Align/add padding to 64 bytes to avoid false sharing
class ITask : public RefCounted<ITask>
{
    // This is commented out because it is wasteful to instance a block allocator for ITask that will never be used.
    // Having the overloads does grant us some asserts in case something goes terribly wrong or user forgets to use
    // their/our overriden new/delete, but it is not worth the cost.
    // KIT_OVERRIDE_NEW_DELETE(ITask, 32)
    KIT_NON_COPYABLE(ITask)
  public:
    struct Range
    {
        usz Begin;
        usz End;
    };

    virtual ~ITask() KIT_NOEXCEPT = default;

    virtual void operator()(usz p_ThreadIndex) KIT_NOEXCEPT = 0;

    bool Valid() const KIT_NOEXCEPT;
    bool Finished() const KIT_NOEXCEPT;
    void WaitUntilFinished() const KIT_NOEXCEPT;

  protected:
    ITask() KIT_NOEXCEPT = default;

    void NotifyCompleted() KIT_NOEXCEPT;

  private:
    TaskManager *m_Manager = nullptr;
    std::atomic_flag m_Finished = ATOMIC_FLAG_INIT;

    friend class TaskManager;
};

// The generic and the specialized could be merged by moving the specialized bit to a base class that is empty when T is
// void, however this use case is simple enough to not warrant the extra complexity.
template <typename T> class Task final : public ITask
{
    // As of right now, block allocator (the default new/delete override) has no thread safe capabilities. This will be
    // commented out until thread safety is added
    // KIT_OVERRIDE_NEW_DELETE(Task<T>, 32)
  public:
    void operator()(const usz p_ThreadIndex) KIT_NOEXCEPT override
    {
        m_Result = m_Function(p_ThreadIndex);
        NotifyCompleted();
    }

    const T &WaitForResult() const KIT_NOEXCEPT
    {
        WaitUntilFinished();
        return m_Result;
    }

  private:
    template <typename Callable, typename... Args>
        requires(!std::is_same_v<Callable, Task>)
    explicit Task(Callable &&p_Callable, Args &&...p_Args) KIT_NOEXCEPT
        : m_Function(std::bind_front(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...))
    {
        KIT_ASSERT(sizeof...(Args) > 0, "Wrong constructor used for Task<T>");
    }

    template <typename Callable>
        requires(!std::is_same_v<Callable, Task>)
    explicit Task(Callable &&p_Callable) KIT_NOEXCEPT : m_Function(std::forward<Callable>(p_Callable))
    {
    }

    std::function<T(usz)> m_Function = nullptr;
    T m_Result;

    friend class TaskManager;
};

template <> class Task<void> final : public ITask
{
    KIT_OVERRIDE_NEW_DELETE(Task<void>, 32)
  public:
    void operator()(usz p_ThreadIndex) KIT_NOEXCEPT override;

    template <typename Callable, typename... Args>
        requires(!std::is_same_v<Callable, Task>)
    explicit Task(Callable &&p_Callable, Args &&...p_Args) KIT_NOEXCEPT
        : m_Function(std::bind_front(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...))
    {
        KIT_ASSERT(sizeof...(Args) > 0, "Wrong constructor used for Task<void>");
    }

    template <typename Callable>
        requires(!std::is_same_v<Callable, Task>)
    explicit Task(Callable &&p_Callable) KIT_NOEXCEPT : m_Function(std::forward<Callable>(p_Callable))
    {
    }

  private:
    std::function<void(usz)> m_Function = nullptr;
    friend class TaskManager;
};

KIT_NAMESPACE_END