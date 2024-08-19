#pragma once

#include "kit/multiprocessing/task.hpp"
#include "kit/memory/ptr.hpp"
#include <type_traits>

KIT_NAMESPACE_BEGIN

class TaskManager
{
  public:
    explicit TaskManager(u32 p_ThreadCount) KIT_NOEXCEPT;
    virtual ~TaskManager() KIT_NOEXCEPT = default;

    // Default implementation just adds the task to the dynamic array
    virtual void SubmitTask(const Ref<ITask> &p_Task) KIT_NOEXCEPT = 0;

    // This has a very big problem: If new is overloaded to use the block allocator, this method stops being thread-safe
    template <typename Callable, typename... Args>
    auto CreateTask(Callable &&p_Callable,
                    Args &&...p_Args) KIT_NOEXCEPT->Ref<Task<std::invoke_result_t<Callable, Args..., usz>>>
    {
        using RType = std::invoke_result_t<Callable, Args..., usz>;
        Task<RType> *task = new Task<RType>(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...);
        task->m_Manager = this;
        return Ref<Task<RType>>(task);
    }

    template <typename Callable, typename... Args>
    auto CreateAndSubmit(Callable &&p_Callable,
                         Args &&...p_Args) KIT_NOEXCEPT->Ref<Task<std::invoke_result_t<Callable, Args..., usz>>>
    {
        using RType = std::invoke_result_t<Callable, Args..., usz>;
        const Ref<Task<RType>> task = CreateTask(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...);
        SubmitTask(task);
        return task;
    }

    u32 ThreadCount() const KIT_NOEXCEPT;

  private:
    u32 m_ThreadCount;
    friend class ITask;
};

KIT_NAMESPACE_END