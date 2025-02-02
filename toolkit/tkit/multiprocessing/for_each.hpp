#pragma once

#include "tkit/multiprocessing/task_manager.hpp"

namespace TKit
{
template <typename T>
concept RandomIterOrIndex = std::integral<T> || std::random_access_iterator<T>;

template <typename T, typename U>
concept OutputIterOrIndex = std::integral<T> || std::output_iterator<T, U>;

namespace Detail
{
template <RandomIterOrIndex It> usize Distance(const It p_First, const It p_Last)
{
    if constexpr (std::integral<It>)
        return static_cast<usize>(p_Last - p_First);
    else
        return static_cast<usize>(std::distance(p_First, p_Last));
}
} // namespace Detail

/**
 * @brief A function that iterates over a range of elements and processes each of them using a task system.
 *
 * It is most useful when used as a way to parallelize a loop, where each iteration is independent of the others.
 * Take into account that the user must await the tasks to finish before going on.
 *
 * @param p_Manager The task manager to use, which must be derived from `ITaskManager`.
 * @param p_First The first iterator or index of the range.
 * @param p_Last The last iterator or index of the range.
 * @param p_Dest An output iterator where the tasks will be stored. It is up to the user to provide a large enough
 * container to store all the tasks through the iterator and to await for them. The amount of tasks created is equal to
 * `p_Partitions`.
 * @param p_Partitions The number of partitions to create.
 * @param p_Callable The callable object to execute. It must be a function object that takes two iterators as arguments
 * (and the mandatory thread index argument at the end as well). It will be called for each element in the range
 * [`p_First`, `p_Last`). The function is called as: `p_Callable(YouArgs..., p_Start, p_End, p_ThreadIndex)`
 * @param p_Args Extra arguments to pass to the callable object. These arguments go before the iterators and thread
 * index.
 */
template <std::derived_from<ITaskManager> TManager, RandomIterOrIndex It1, OutputIterOrIndex<Ref<ITask>> It2,
          typename Callable, typename... Args>
void ForEach(TManager &p_Manager, const It1 p_First, const It1 p_Last, It2 p_Dest, const usize p_Partitions,
             Callable &&p_Callable, Args &&...p_Args)
{
    const usize size = Detail::Distance(p_First, p_Last);
    usize start = 0;
    for (usize i = 0; i < p_Partitions; ++i)
    {
        const usize end = (i + 1) * size / p_Partitions;
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

/**
 * @brief A function that iterates over a range of elements and processes each of them using a task system.
 *
 * It is most useful when used as a way to parallelize a loop, where each iteration is independent of the others.
 * The main difference with the other overload is that this one does not store the tasks in an output iterator. This is
 * useful when the tasks do not return a value. The user still needs to await for the tasks to finish before going on.
 *
 * @param p_Manager The task manager to use, which must be derived from `ITaskManager`.
 * @param p_First The first iterator or index of the range.
 * @param p_Last The last iterator or index of the range.
 * @param p_Partitions The number of partitions to create.
 * @param p_Callable The callable object to execute. It must be a function object that takes two iterators as arguments
 * (and the mandatory thread index argument at the end as well). It will be called for each element in the range
 * [`p_First`, `p_Last`). The function is called as: `p_Callable(YouArgs..., p_Start, p_End, p_ThreadIndex)`
 * @param p_Args Extra arguments to pass to the callable object. These arguments go before the iterators and thread
 * index.
 */
template <std::derived_from<ITaskManager> TManager, RandomIterOrIndex It, typename Callable, typename... Args>
void ForEach(TManager &p_Manager, const It p_First, const It p_Last, const usize p_Partitions, Callable &&p_Callable,
             Args &&...p_Args)
{
    const usize size = Detail::Distance(p_First, p_Last);
    usize start = 0;
    for (usize i = 0; i < p_Partitions; ++i)
    {
        const usize end = (i + 1) * size / p_Partitions;
        TKIT_ASSERT(end <= size, "[TOOLKIT] Partition exceeds container size");
        p_Manager.CreateAndSubmit(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)..., p_First + start,
                                  p_First + end);
        start = end;
    }
}

/**
 * @brief A function that iterates over a range of elements and processes each of them using a task system.
 *
 * It is most useful when used as a way to parallelize a loop, where each iteration is independent of the others.
 * Take into account that the user must await the tasks to finish before going on.
 *
 * This version of the function will let the main thread execute the first task/partition.
 *
 * @param p_Manager The task manager to use, which must be derived from `ITaskManager`.
 * @param p_First The first iterator or index of the range.
 * @param p_Last The last iterator or index of the range.
 * @param p_Dest An output iterator where the tasks will be stored. It is up to the user to provide a large enough
 * container to store all the tasks through the iterator and to await for them. The amount of tasks created is equal to
 * `p_Partitions - 1` (because the main thread will execute the first task/partition).
 * @param p_OutRet A pointer to a variable where the return value of the main thread task will be stored.
 * @param p_Partitions The number of partitions to create.
 * @param p_Callable The callable object to execute. It must be a function object that takes two iterators as arguments
 * (and the mandatory thread index argument at the end as well). It will be called for each element in the range
 * [`p_First`, `p_Last`). The function is called as: `p_Callable(YouArgs..., p_Start, p_End, p_ThreadIndex)`
 * @param p_Args Extra arguments to pass to the callable object. These arguments go before the iterators and thread
 * index.
 */
template <std::derived_from<ITaskManager> TManager, RandomIterOrIndex It1, OutputIterOrIndex<Ref<ITask>> It2,
          typename Callable, typename... Args>
auto ForEachMainThreadLead(TManager &p_Manager, const It1 p_First, const It1 p_Last, It2 p_Dest,
                           const usize p_Partitions, Callable &&p_Callable, Args &&...p_Args)
    -> std::invoke_result_t<Callable, Args..., It1, It1, usize>
{
    const usize size = Detail::Distance(p_First, p_Last);
    usize start = size / p_Partitions;
    for (usize i = 1; i < p_Partitions; ++i)
    {
        const usize end = (i + 1) * size / p_Partitions;
        TKIT_ASSERT(end <= size, "[TOOLKIT] Partition exceeds container size");
        if (end > start)
        {
            *p_Dest = p_Manager.CreateAndSubmit(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...,
                                                p_First + start, p_First + end);
            ++p_Dest;
        }
        start = end;
    }
    const usize end = size / p_Partitions;
    return p_Callable(std::forward<Args>(p_Args)..., p_First, p_First + end, 0);
}

/**
 * @brief A function that iterates over a range of elements and processes each of them using a task system.
 *
 * It is most useful when used as a way to parallelize a loop, where each iteration is independent of the others.
 * The main difference with the other overload is that this one does not store the tasks in an output iterator. This is
 * useful when the tasks do not return a value. The user still needs to await for the tasks to finish before going on.
 *
 * This version of the function will let the main thread execute the first task of each partition.
 *
 * @param p_Manager The task manager to use, which must be derived from `ITaskManager`.
 * @param p_First The first iterator or index of the range.
 * @param p_Last The last iterator or index of the range.
 * @param p_Partitions The number of partitions to create.
 * @param p_Callable The callable object to execute. It must be a function object that takes two iterators as arguments
 * (and the mandatory thread index argument at the end as well). It will be called for each element in the range
 * [`p_First`, `p_Last`). The function is called as: `p_Callable(YouArgs..., p_Start, p_End, p_ThreadIndex)`
 * @param p_Args Extra arguments to pass to the callable object. These arguments go before the iterators and thread
 * index.
 */
template <std::derived_from<ITaskManager> TManager, RandomIterOrIndex It, typename Callable, typename... Args>
void ForEachMainThreadLead(TManager &p_Manager, const It p_First, const It p_Last, const usize p_Partitions,
                           Callable &&p_Callable, Args &&...p_Args)
{
    const usize size = Detail::Distance(p_First, p_Last);
    usize start = size / p_Partitions;
    for (usize i = 1; i < p_Partitions; ++i)
    {
        const usize end = (i + 1) * size / p_Partitions;
        TKIT_ASSERT(end <= size, "[TOOLKIT] Partition exceeds container size");
        p_Manager.CreateAndSubmit(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)..., p_First + start,
                                  p_First + end);
        start = end;
    }
    const usize end = size / p_Partitions;
    p_Callable(std::forward<Args>(p_Args)..., p_First, p_First + end, 0);
}
} // namespace TKit