#include "core/pch.hpp"
#include "kit/multiprocessing/spin_mutex.hpp"

namespace KIT
{
void SpinMutex::lock() noexcept
{
    while (m_Flag.test_and_set(std::memory_order_relaxed))
        std::this_thread::yield();
    std::atomic_thread_fence(std::memory_order_acquire);
}

void SpinMutex::unlock() noexcept
{
    m_Flag.clear(std::memory_order_release);
}

bool SpinMutex::try_lock() noexcept
{
    return !m_Flag.test_and_set(std::memory_order_acquire);
}
} // namespace KIT