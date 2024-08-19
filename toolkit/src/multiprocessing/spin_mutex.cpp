#include "core/pch.hpp"
#include "kit/multiprocessing/spin_mutex.hpp"

KIT_NAMESPACE_BEGIN

void SpinMutex::lock() KIT_NOEXCEPT
{
    while (m_Flag.test_and_set(std::memory_order_acquire))
        ;
}

void SpinMutex::unlock() KIT_NOEXCEPT
{
    m_Flag.clear(std::memory_order_release);
}

bool SpinMutex::try_lock() KIT_NOEXCEPT
{
    return !m_Flag.test_and_set(std::memory_order_acquire);
}

KIT_NAMESPACE_END