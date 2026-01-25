#pragma once

#include "tkit/multiprocessing/task_manager.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit::Detail
{
template <typename It> constexpr usize Distance(const It first, const It last)
{
    if constexpr (std::integral<It>)
        return static_cast<usize>(last - first);
    else
        return static_cast<usize>(std::distance(first, last));
}
} // namespace TKit::Detail

namespace TKit
{
/**
 * @brief A function that iterates over a range of elements and processes each of them using a task system.
 *
 * It is most useful when used as a way to parallelize a loop, where each iteration is independent of the others.
 * Users may choose when to await each partition through the returned tasks.
 *
 * This function delegates all tasks to the threads of the task manager. The caller thread will not be assigned a task
 * except if it belongs to the passed task manager.
 *
 * @param manager The task manager to use, which must be derived from `ITaskManager`.
 * @param first The first iterator or index of the range.
 * @param last The last iterator or index of the range.
 * @param dest An output iterator where the tasks will be stored. It is up to the user to provide a large enough
 * container to store all the tasks through the iterator and to await for them. The amount of tasks created is equal to
 * `partitions`.
 * @param partitions The number of partitions to create.
 * @param callable The callable object to execute. It must be a function object that takes two iterators as arguments
 * (and the mandatory thread index argument at the end as well). It will be called for each element in the range
 * [`first`, `last`). The function is called as: `callable(start, end, YouArgs...)`
 * @param args Extra arguments to pass to the callable object. These arguments go before the iterators and thread
 * index.
 */
template <std::derived_from<ITaskManager> TManager, typename It1, typename It2, typename Callable, typename... Args>
void NonBlockingForEach(TManager &manager, const It1 first, const It1 last, It2 dest, const usize partitions,
                        Callable &&callable, Args &&...args)
{
    const usize size = Detail::Distance(first, last);
    usize start = 0;
    usize sindex = 0;

    for (usize i = 0; i < partitions; ++i)
    {
        const usize end = (i + 1) * size / partitions;
        TKIT_ASSERT(end <= size, "[TOOLKIT][FOR-EACH] Partition exceeds container size");
        auto &task = *(dest++);
        task.Set(std::forward<Callable>(callable), first + start, first + end, std::forward<Args>(args)...);

        sindex = manager.SubmitTask(&task, sindex);
        start = end;
    }
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
 * @param manager The task manager to use, which must be derived from `ITaskManager`.
 * @param first The first iterator or index of the range.
 * @param last The last iterator or index of the range.
 * @param dest An output iterator where the tasks will be stored. It is up to the user to provide a large enough
 * container to store all the tasks through the iterator and to await for them. The amount of tasks created is equal to
 * `partitions - 1` (because the caller thread will execute the first task/partition).
 * @param partitions The number of partitions to create. This number takes into account the main thread.
 * @param callable The callable object to execute. It must be a function object that takes two iterators as arguments
 * (and the mandatory thread index argument at the end as well). It will be called for each element in the range
 * [`first`, `last`). The function is called as: `callable(start, end, YouArgs...)`
 * @param args Extra arguments to pass to the callable object. These arguments go before the iterators and thread
 * index.
 */
template <std::derived_from<ITaskManager> TManager, typename It1, typename It2, typename Callable, typename... Args>
auto BlockingForEach(TManager &manager, const It1 first, const It1 last, It2 dest, const usize partitions,
                     Callable &&callable, Args &&...args) -> std::invoke_result_t<Callable, Args..., It1, It1>
{
    const usize size = Detail::Distance(first, last);
    usize start = size / partitions;
    if (partitions == 1)
        return callable(first, first + start, std::forward<Args>(args)...);

    usize sindex = 0;
    for (usize i = 1; i < partitions; ++i)
    {
        const usize end = (i + 1) * size / partitions;
        TKIT_ASSERT(end <= size, "[TOOLKIT][FOR-EACH] Partition exceeds container size");

        auto &task = *(dest++);
        task.Set(std::forward<Callable>(callable), first + start, first + end, std::forward<Args>(args)...);
        sindex = manager.SubmitTask(&task, sindex);

        start = end;
    }

    const usize end = size / partitions;
    return callable(first, first + end, std::forward<Args>(args)...);
}
} // namespace TKit
