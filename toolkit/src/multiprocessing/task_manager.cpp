#include "core/pch.hpp"
#include "kit/multiprocessing/task_manager.hpp"

namespace KIT
{
TaskManager::TaskManager(const usize p_ThreadCount) KIT_NOEXCEPT : m_ThreadCount(p_ThreadCount)
{
}

usize TaskManager::ThreadCount() const KIT_NOEXCEPT
{
    return m_ThreadCount;
}
} // namespace KIT