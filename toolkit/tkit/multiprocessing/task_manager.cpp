#include "tkit/core/pch.hpp"
#include "tkit/multiprocessing/task_manager.hpp"

namespace TKit
{
ITaskManager::ITaskManager(const usize p_ThreadCount) noexcept : m_ThreadCount(p_ThreadCount)
{
}

usize ITaskManager::GetThreadCount() const noexcept
{
    return m_ThreadCount;
}

usize ITaskManager::GetThreadIndex() const noexcept
{
    return 0;
}
} // namespace TKit