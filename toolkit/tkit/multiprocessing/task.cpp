#include "tkit/core/pch.hpp"
#include "tkit/multiprocessing/task.hpp"

#ifndef TKIT_TASK_ALLOCATOR_CAPACITY
#    define TKIT_TASK_ALLOCATOR_CAPACITY 1024
#endif

namespace TKit
{
bool ITask::IsFinished(const std::memory_order p_Order) const
{
    return m_Finished.test(p_Order);
}
void ITask::WaitUntilFinished() const
{
    m_Finished.wait(false, std::memory_order_acquire);
}
void ITask::Reset()
{
    m_Finished.clear(std::memory_order_relaxed);
}

void ITask::notifyCompleted()
{
#ifdef TKIT_ENABLE_ASSERTS
    const bool flag = m_Finished.test_and_set(std::memory_order_release);
    TKIT_ASSERT(!flag, "[TOOLKIT][TASK] Notifying an already completed task");
#else
    m_Finished.test_and_set(std::memory_order_release);
#endif
    m_Finished.notify_all();
}

void Task<void>::operator()()
{
    m_Function();
    notifyCompleted();
}
} // namespace TKit
