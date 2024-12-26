#pragma once

#include "tkit/multiprocessing/task_manager.hpp"

namespace TKit
{

/**
 * @brief A function that iterates over a range of elements and processes each of them using a task system.
 *
 * It is most useful when used as a way to parallelize a loop, where each iteration is independent of the others.
 *
 * @param p_Manager The task manager to use, which must be derived from ITaskManager.
 * @param p_First The first element of the range.
 * @param p_Last The last element of the range.
 * @param p_Dest An output iterator where the tasks will be stored. It is up to the user to provide a large enough
 * container to store all the tasks through the iterator. The amount of tasks created is equal to p_Tasks.
 * @param p_Tasks The number of tasks to create.
 * @param p_Callable The callable object to execute. It must be a function object that takes two iterators as arguments
 * (and the mandatory thread index argument at the end as well). It will be called for each element in the range
 * [p_First, p_Last).
 * @param p_Args Extra arguments to pass to the callable object. These arguments go before the iterators and thread
 * index.
 */
template <std::derived_from<ITaskManager> TManager, std::random_access_iterator It1,
          std::output_iterator<Ref<ITask>> It2, typename Callable, typename... Args>
void ForEach(TManager &p_Manager, It1 p_First, It1 p_Last, It2 p_Dest, const usize p_Tasks, Callable &&p_Callable,
             Args &&...p_Args)
{
    const usize size = std::distance(p_First, p_Last);
    usize start = 0;
    for (usize i = 0; i < p_Tasks; ++i)
    {
        const usize end = (i + 1) * size / p_Tasks;
        TKIT_ASSERT(end <= size, "[TOOLKIT] Partition exceeds container size");
        if (end > start)
        {
            *p_Dest = p_Manager.CreateAndSubmit(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...,
                                                p_First + start, p_First + end);
            ++p_Dest;
        }
        start = end;
    }
}
} // namespace TKit