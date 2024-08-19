#include "core/pch.hpp"
#include "kit/multiprocessing/task_manager.hpp"

KIT_NAMESPACE_BEGIN

TaskManager::TaskManager(u32 p_ThreadCount) : m_ThreadCount(p_ThreadCount)
{
}

u32 TaskManager::ThreadCount() const KIT_NOEXCEPT
{
    return m_ThreadCount;
}

KIT_NAMESPACE_END