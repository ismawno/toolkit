#include "core/pch.hpp"
#include "kit/multiprocessing/spin_lock.hpp"

namespace KIT
{
void SpinLock::lock() noexcept
{
    while (m_Flag.test_and_set(std::memory_order_acquire))
        std::this_thread::yield();
}

void SpinLock::unlock() noexcept
{
    m_Flag.clear(std::memory_order_release);
}

bool SpinLock::try_lock() noexcept
{
    return !m_Flag.test_and_set(std::memory_order_acquire);
}
} // namespace KIT