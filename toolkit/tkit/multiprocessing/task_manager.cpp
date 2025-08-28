#include "tkit/core/pch.hpp"
#include "tkit/utils/logging.hpp"
#include "tkit/multiprocessing/task_manager.hpp"

namespace TKit
{
ITaskManager::ITaskManager(const usize p_WorkerCount) noexcept : m_WorkerCount(p_WorkerCount)
{
    TKIT_ASSERT(p_WorkerCount != 0, "The thread count must be greater than 0.");
}

usize ITaskManager::GetWorkerCount() const noexcept
{
    return m_WorkerCount;
}

usize ITaskManager::GetThreadIndex() noexcept
{
    return s_ThreadIndex;
}
} // namespace TKit
