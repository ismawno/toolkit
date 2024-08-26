#pragma once

#include "kit/core/core.hpp"
#include <atomic>

namespace KIT
{
class KIT_API SpinMutex
{
  public:
    void lock() noexcept;
    void unlock() noexcept;
    bool try_lock() noexcept;

  private:
    std::atomic_flag m_Flag = ATOMIC_FLAG_INIT;
};
} // namespace KIT