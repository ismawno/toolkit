#include "tkit/core/pch.hpp"
#include "tkit/utils/debug.hpp"
#include "tkit/multiprocessing/task.hpp"

namespace TKit
{
void ITask::WaitUntilFinished() const
{
    m_Finished.wait(false, std::memory_order_acquire);
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
