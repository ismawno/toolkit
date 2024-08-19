#pragma once

#include "kit/multiprocessing/task_manager.hpp"

KIT_NAMESPACE_BEGIN

template <typename TManager, std::random_access_iterator It1, std::output_iterator<Ref<ITask>> It2, typename Callable,
          typename... Args>
    requires std::is_base_of_v<TaskManager, TManager>
void ForEach(TManager &p_Manager, It1 p_First, It1 p_Last, It2 p_Dest, const u32 p_Tasks, Callable &&p_Callable,
             Args &&...p_Args)
{
    const usz size = std::distance(p_First, p_Last);
    u32 start = 0;
    for (u32 i = 0; i < p_Tasks; ++i)
    {
        const u32 end = (i + 1) * size / p_Tasks;
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

KIT_NAMESPACE_END