#pragma once

#include "kit/multiprocessing/task.hpp"
#include <type_traits>

namespace KIT
{
// A simple TaskManager interface that allows users to implement their own task system with their own threading model
// TODO: A task may only be submitted again if it has finished execution
class KIT_API TaskManager
{
  public:
    explicit TaskManager(usize p_ThreadCount) noexcept;
    virtual ~TaskManager() noexcept = default;

    virtual void SubmitTask(const Ref<ITask> &p_Task) noexcept = 0;

    template <typename Callable, typename... Args>
    auto CreateTask(Callable &&p_Callable, Args &&...p_Args) noexcept
        -> Ref<Task<std::invoke_result_t<Callable, Args..., usize>>>
    {
        using RType = std::invoke_result_t<Callable, Args..., usize>;
        return Ref<Task<RType>>::Create(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...);
    }

    template <typename Callable, typename... Args>
    auto CreateAndSubmit(Callable &&p_Callable, Args &&...p_Args) noexcept
        -> Ref<Task<std::invoke_result_t<Callable, Args..., usize>>>
    {
        using RType = std::invoke_result_t<Callable, Args..., usize>;
        const Ref<Task<RType>> task = CreateTask(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...);
        SubmitTask(task);
        return task;
    }

    usize ThreadCount() const noexcept;

  private:
    usize m_ThreadCount;
    friend class ITask;
};
} // namespace KIT