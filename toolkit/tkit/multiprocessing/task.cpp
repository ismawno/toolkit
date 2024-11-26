#include "tkit/core/pch.hpp"
#include "tkit/multiprocessing/task.hpp"
#include "tkit/multiprocessing/task_manager.hpp"

namespace TKit
{
bool ITask::IsFinished() const noexcept
{
    return m_Finished.test(std::memory_order_relaxed);
}
void ITask::WaitUntilFinished() const noexcept
{
    m_Finished.wait(false, std::memory_order_acquire);
}
void ITask::Reset() noexcept
{
    m_Finished.clear(std::memory_order_relaxed);
}

void ITask::NotifyCompleted() noexcept
{
#ifdef TKIT_ENABLE_ASSERTS
    const bool flag = m_Finished.test_and_set(std::memory_order_release);
    TKIT_ASSERT(!flag, "Notifying an already completed task");
#else
    m_Finished.test_and_set(std::memory_order_release);
#endif
    m_Finished.notify_all();
}

void Task<void>::operator()(const usize p_ThreadIndex) noexcept
{
    m_Function(p_ThreadIndex);
    NotifyCompleted();
}
} // namespace TKit