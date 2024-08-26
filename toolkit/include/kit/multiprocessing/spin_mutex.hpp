#pragma once

#include "kit/core/core.hpp"
#include <atomic>

namespace KIT
{
class KIT_API SpinMutex
{
  public:
    void lock() KIT_NOEXCEPT;
    void unlock() KIT_NOEXCEPT;
    bool try_lock() KIT_NOEXCEPT;

  private:
    std::atomic_flag m_Flag = ATOMIC_FLAG_INIT;
};
} // namespace KIT