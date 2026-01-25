#include "tkit/core/pch.hpp"
#include "tkit/utils/debug.hpp"
#include "tkit/multiprocessing/task_manager.hpp"

namespace TKit
{
ITaskManager::ITaskManager(const usize workerCount) : m_WorkerCount(workerCount)
{
    TKIT_ASSERT(workerCount != 0, "[TOOLKIT][MULTIPROC] The worker count must be greater than 0");
}

TaskManager::TaskManager() : ITaskManager(1)
{
}

usize TaskManager::SubmitTask(ITask *task, const usize)
{
    (*task)();
    return 0;
}
void TaskManager::WaitUntilFinished(const ITask &task)
{
    task.WaitUntilFinished();
}
} // namespace TKit
