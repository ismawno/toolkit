#pragma once

#include "tkit/core/api.hpp"
#include <atomic>

namespace TKit
{
/**
 * @brief A simple spin lock that uses atomic operations to lock and unlock.
 *
 * It is a very simple lock that is useful for short critical sections. It is not recommended for long critical
 * sections, as it can cause a lot of contention.
 *
 */
class TKIT_API SpinLock
{
  public:
    void lock() noexcept;
    void unlock() noexcept;
    bool try_lock() noexcept;

  private:
    std::atomic_flag m_Flag = ATOMIC_FLAG_INIT;
};
} // namespace TKit