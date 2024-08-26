#pragma once

#include "kit/core/concepts.hpp"
#include "kit/multiprocessing/task_manager.hpp"
#include <thread>

namespace KIT
{
// An implementation of a TaskManager that uses a thread pool to execute tasks. Currently, queue synchronization is
// achieved by a plain mutex lock, which is not ideal. I have considered implementing a lock-free approach, but I wont
// do it until the need arises, as those are very complex to get right

// TODO: Implement a lock-free queue
// TODO: Consider adding dependencies
template <Mutex MTX> class ThreadPool final : public TaskManager
{
  public:
    explicit ThreadPool(usize p_ThreadCount);

    void SubmitTask(const Ref<ITask> &p_Task) noexcept override;
    void AwaitPendingTasks() const noexcept;

    ~ThreadPool() noexcept override;

  private:
    DynamicArray<std::thread> m_Threads;
    Deque<Ref<ITask>> m_Queue;

    std::atomic_flag m_TaskReady = ATOMIC_FLAG_INIT;
    std::atomic_flag m_Shutdown = ATOMIC_FLAG_INIT;

    std::atomic<u32> m_TerminatedCount = 0;
    std::atomic<u32> m_PendingCount = 0;

    mutable MTX m_Mutex;
};
} // namespace KIT