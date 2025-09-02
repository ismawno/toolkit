#include "tkit/core/pch.hpp"
#include "tkit/utils/logging.hpp"
#include "tkit/multiprocessing/task_manager.hpp"

namespace TKit
{
ITaskManager::ITaskManager(const usize p_WorkerCount) noexcept : m_WorkerCount(p_WorkerCount)
{
    TKIT_ASSERT(p_WorkerCount != 0, "[TOOLKIT][MULTIPROC] The worker count must be greater than 0.");
}

usize ITaskManager::GetWorkerCount() const noexcept
{
    return m_WorkerCount;
}

usize ITaskManager::GetThreadIndex() noexcept
{
    return t_ThreadIndex;
}

TaskManager::TaskManager() noexcept : ITaskManager(1)
{
}

void TaskManager::SubmitTask(ITask *p_Task) noexcept
{
    (*p_Task)();
}
void TaskManager::SubmitTasks(const Span<ITask *const> p_Tasks) noexcept
{
    for (ITask *task : p_Tasks)
        (*task)();
}
void TaskManager::WaitUntilFinished(ITask *p_Task) noexcept
{
    p_Task->WaitUntilFinished();
}
} // namespace TKit
