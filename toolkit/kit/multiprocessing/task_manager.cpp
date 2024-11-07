#include "kit/core/pch.hpp"
#include "kit/multiprocessing/task_manager.hpp"

namespace KIT
{
ITaskManager::ITaskManager(const usize p_ThreadCount) noexcept : m_ThreadCount(p_ThreadCount)
{
}

usize ITaskManager::GetThreadCount() const noexcept
{
    return m_ThreadCount;
}
} // namespace KIT