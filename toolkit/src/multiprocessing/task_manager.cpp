#include "core/pch.hpp"
#include "kit/multiprocessing/task_manager.hpp"

namespace KIT
{
TaskManager::TaskManager(const usize p_ThreadCount) noexcept : m_ThreadCount(p_ThreadCount)
{
}

usize TaskManager::ThreadCount() const noexcept
{
    return m_ThreadCount;
}
} // namespace KIT