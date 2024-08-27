#include "core/pch.hpp"
#include "kit/multiprocessing/task.hpp"
#include "kit/multiprocessing/task_manager.hpp"

namespace KIT
{
bool ITask::Valid() const noexcept
{
    return m_Manager != nullptr;
}

bool ITask::Finished() const noexcept
{
    return Valid() && m_Finished.test(std::memory_order_relaxed);
}

void ITask::WaitUntilFinished() const noexcept
{
    m_Finished.wait(false, std::memory_order_acquire);
}

void ITask::NotifyCompleted() noexcept
{
#ifdef KIT_ENABLE_ASSERTS
    const bool flag = m_Finished.test_and_set(std::memory_order_release);
    KIT_ASSERT(!flag, "Notifying an already completed task");
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
} // namespace KIT