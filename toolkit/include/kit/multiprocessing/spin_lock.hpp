#pragma once

#include "kit/core/api.hpp"
#include <atomic>

namespace KIT
{
class KIT_API SpinLock
{
  public:
    void lock() noexcept;
    void unlock() noexcept;
    bool try_lock() noexcept;

  private:
    std::atomic_flag m_Flag = ATOMIC_FLAG_INIT;
};
} // namespace KIT