#include "tkit/core/pch.hpp"
#include "tkit/utils/logging.hpp"
#include "tkit/multiprocessing/task_manager.hpp"

namespace TKit
{
ITaskManager::ITaskManager(const usize p_ThreadCount) noexcept : m_ThreadCount(p_ThreadCount)
{
    TKIT_ASSERT(p_ThreadCount != 0, "The thread count must be greater than 0.");
}

usize ITaskManager::GetThreadCount() const noexcept
{
    return m_ThreadCount;
}

usize ITaskManager::GetThreadIndex() noexcept
{
    return s_ThreadIndex;
}
} // namespace TKit
