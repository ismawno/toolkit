#pragma once

#include "tkit/multiprocessing/task_manager.hpp"
#include "tkit/container/span.hpp"
#include "tkit/utils/concepts.hpp"

namespace TKit
{
// template <typename T>
// concept RandomIterOrIndex = std::integral<T> || std::random_access_iterator<T>;

// template <typename T, typename U>
// concept OutputIterOrIndex = std::integral<T> || std::output_iterator<T, U>;

namespace Detail
{
template <typename It> usize Distance(const It p_First, const It p_Last) noexcept
{
    if constexpr (std::integral<It>)
        return static_cast<usize>(p_Last - p_First);
    else
        return static_cast<usize>(std::distance(p_First, p_Last));
}
template <typename It> auto CreateSpan(const It p_First, const usize p_Size) noexcept
{
    using T = NoCVRef<decltype(*p_First)>;
    if constexpr (std::is_pointer_v<It>)
        return Span<const T>{p_First, p_Size};
    else
        return Span<const T>{&*p_First, p_Size};
}
} // namespace Detail

/**
 * @brief A function that iterates over a range of elements and processes each of them using a task system.
 *
 * It is most useful when used as a way to parallelize a loop, where each iteration is independent of the others.
 * Users may choose when to await each partition through the returned tasks.
 *
 * This function delegates all tasks to the threads of the task manager. The caller thread will not be assigned a task
 * except if it belongs to the passed task manager.
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
template <std::derived_from<ITaskManager> TManager, typename It1, typename It2, typename Callable, typename... Args>
void NonBlockingForEach(TManager &p_Manager, const It1 p_First, const It1 p_Last, It2 p_Dest, const usize p_Partitions,
                        Callable &&p_Callable, Args &&...p_Args)
{
    const usize size = Detail::Distance(p_First, p_Last);
    usize start = 0;

    const auto tasks = Detail::CreateSpan(p_Dest, p_Partitions);

    for (usize i = 0; i < p_Partitions; ++i)
    {
        const usize end = (i + 1) * size / p_Partitions;
        TKIT_ASSERT(end <= size, "[TOOLKIT][FOR-EACH] Partition exceeds container size");
        *p_Dest = p_Manager.CreateTask(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...,
                                       p_First + start, p_First + end);
        ++p_Dest;
        start = end;
    }
    p_Manager.SubmitTasks(tasks);
}

/**
 * @brief A function that iterates over a range of elements and processes each of them using a task system.
 *
 * It is most useful when used as a way to parallelize a loop, where each iteration is independent of the others.
 * Users may choose when to await each partition through the returned tasks.
 *
 * This function will explicitly assign a task to the caller thread. If the caller thread already belongs to the passed
 * task manager, it may end up with double the workload.
 *
 * @param p_Manager The task manager to use, which must be derived from `ITaskManager`.
 * @param p_First The first iterator or index of the range.
 * @param p_Last The last iterator or index of the range.
 * @param p_Dest An output iterator where the tasks will be stored. It is up to the user to provide a large enough
 * container to store all the tasks through the iterator and to await for them. The amount of tasks created is equal to
 * `p_Partitions - 1` (because the caller thread will execute the first task/partition).
 * @param p_Partitions The number of partitions to create. This number takes into account the main thread.
 * @param p_Callable The callable object to execute. It must be a function object that takes two iterators as arguments
 * (and the mandatory thread index argument at the end as well). It will be called for each element in the range
 * [`p_First`, `p_Last`). The function is called as: `p_Callable(YouArgs..., p_Start, p_End, p_ThreadIndex)`
 * @param p_Args Extra arguments to pass to the callable object. These arguments go before the iterators and thread
 * index.
 */
template <std::derived_from<ITaskManager> TManager, typename It1, typename It2, typename Callable, typename... Args>
auto BlockingForEach(TManager &p_Manager, const It1 p_First, const It1 p_Last, It2 p_Dest, const usize p_Partitions,
                     Callable &&p_Callable, Args &&...p_Args) -> std::invoke_result_t<Callable, Args..., It1, It1>
{
    const usize size = Detail::Distance(p_First, p_Last);
    usize start = size / p_Partitions;
    if (p_Partitions == 1)
        return p_Callable(std::forward<Args>(p_Args)..., p_First, p_First + start);

    const auto tasks = Detail::CreateSpan(p_Dest, p_Partitions - 1);
    for (usize i = 1; i < p_Partitions; ++i)
    {
        const usize end = (i + 1) * size / p_Partitions;
        TKIT_ASSERT(end <= size, "[TOOLKIT][FOR-EACH] Partition exceeds container size");

        *p_Dest = p_Manager.CreateTask(std::forward<Callable>(p_Callable), std::forward<Args>(p_Args)...,
                                       p_First + start, p_First + end);
        ++p_Dest;
        start = end;
    }
    p_Manager.SubmitTasks(tasks);

    const usize end = size / p_Partitions;
    return p_Callable(std::forward<Args>(p_Args)..., p_First, p_First + end);
}
} // namespace TKit
