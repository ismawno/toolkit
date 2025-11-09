#include "tkit/core/pch.hpp"
#include "tkit/utils/debug.hpp"
#include "tkit/multiprocessing/task_manager.hpp"

namespace TKit
{
ITaskManager::ITaskManager(const usize p_WorkerCount) : m_WorkerCount(p_WorkerCount)
{
    TKIT_ASSERT(p_WorkerCount != 0, "[TOOLKIT][MULTIPROC] The worker count must be greater than 0.");
}

TaskManager::TaskManager() : ITaskManager(1)
{
}

usize TaskManager::SubmitTask(ITask *p_Task, const usize)
{
    (*p_Task)();
    return 0;
}
void TaskManager::WaitUntilFinished(const ITask &p_Task)
{
    p_Task.WaitUntilFinished();
}
} // namespace TKit
