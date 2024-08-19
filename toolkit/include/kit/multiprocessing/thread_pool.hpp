#pragma once

#include "kit/multiprocessing/task_manager.hpp"
#include <thread>

KIT_NAMESPACE_BEGIN

class ThreadPool final : public TaskManager
{
  public:
    explicit ThreadPool(u32 p_ThreadCount);

    void SubmitTask(const Ref<ITask> &p_Task) KIT_NOEXCEPT override;

    ~ThreadPool() KIT_NOEXCEPT override;

  private:
    DynamicArray<std::thread> m_Threads;
    Deque<Ref<ITask>> m_Queue;

    std::atomic_flag m_TaskReady = ATOMIC_FLAG_INIT;
    std::atomic_flag m_Shutdown = ATOMIC_FLAG_INIT;

    std::atomic<u32> m_TerminatedCount = 0;
    std::atomic<u32> m_PendingCount = 0;

    mutable std::mutex m_Mutex;
};

KIT_NAMESPACE_END