#pragma once

#include "kit/multiprocessing/task_manager.hpp"

namespace KIT
{
template <typename TManager, std::random_access_iterator It1, std::output_iterator<Ref<ITask>> It2, typename Callable,
          typename... Args>
    requires std::is_base_of_v<TaskManager, TManager>
void ForEach(TManager &p_Manager, It1 p_First, It1 p_Last, It2 p_Dest, const usize p_Tasks, Callable &&p_Callable,
             Args &&...p_Args)
{
    const usize size = std::distance(p_First, p_Last);
    usize start = 0;
    for (usize i = 0; i < p_Tasks; ++i)
    {
        const usize end = (i + 1) * size / p_Tasks;
        KIT_ASSERT(end <= size, "Partition exceeds container size");
        if (end > start)
        {
            *p_Dest = p_Manager.CreateAndSubmit(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...,
                                                p_First + start, p_First + end);
            ++p_Dest;
        }
        start = end;
    }
}
} // namespace KIT