#pragma once

#include "kit/core/core.hpp"
#include "kit/core/concepts.hpp"
#include <chrono>

KIT_NAMESPACE_BEGIN

class KIT_API Timespan
{
  public:
    using Nanoseconds = std::chrono::nanoseconds;
    using Microseconds = std::chrono::microseconds;
    using Milliseconds = std::chrono::milliseconds;
    using Seconds = std::chrono::seconds;

    explicit(false) Timespan(Nanoseconds p_Elapsed = Nanoseconds::zero()) KIT_NOEXCEPT;

    template <typename TimeUnit, Numeric T = f32> T As() const KIT_NOEXCEPT
    {
        if constexpr (Float<T>)
            return std::chrono::duration<T, typename TimeUnit::period>(m_Elapsed).count();
        else
        {
            const auto asTimeUnit = std::chrono::duration_cast<TimeUnit>(m_Elapsed);
            return std::chrono::duration<T, typename TimeUnit::period>(asTimeUnit).count();
        }
    }

    template <Numeric T = u64> T AsNanoseconds() const KIT_NOEXCEPT
    {
        return As<Nanoseconds, T>();
    }
    template <Numeric T = f32> T AsMicroseconds() const KIT_NOEXCEPT
    {
        return As<Microseconds, T>();
    }
    template <Numeric T = f32> T AsMilliseconds() const KIT_NOEXCEPT
    {
        return As<Milliseconds, T>();
    }
    template <Numeric T = f32> T AsSeconds() const KIT_NOEXCEPT
    {
        return As<Seconds, T>();
    }

    template <typename TimeUnit, Numeric T> static Timespan From(const T p_Elapsed) KIT_NOEXCEPT
    {
        return Timespan(std::chrono::duration<T, typename TimeUnit::period>(p_Elapsed));
    }

    static void Sleep(Timespan p_Duration) KIT_NOEXCEPT;

    std::strong_ordering operator<=>(const Timespan &other) const = default;

    friend Timespan operator+(const Timespan &lhs, const Timespan &rhs)
    {
        return Timespan(lhs.m_Elapsed + rhs.m_Elapsed);
    }
    friend Timespan operator-(const Timespan &lhs, const Timespan &rhs)
    {
        return Timespan(lhs.m_Elapsed - rhs.m_Elapsed);
    }

    template <Numeric T> friend Timespan operator*(const T scalar, const Timespan &timespan)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(timespan.m_Elapsed * scalar));
    }
    template <Numeric T> friend Timespan operator/(const T scalar, const Timespan &timespan)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(scalar / timespan.m_Elapsed));
    }

    template <Numeric T> friend Timespan operator*(const Timespan &timespan, const T scalar)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(timespan.m_Elapsed * scalar));
    }
    template <Numeric T> friend Timespan operator/(const Timespan &timespan, const T scalar)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(timespan.m_Elapsed / scalar));
    }

    Timespan &operator+=(const Timespan &other);
    Timespan &operator-=(const Timespan &other);

    template <Numeric T> Timespan &operator*=(const T scalar)
    {
        m_Elapsed *= scalar;
        return *this;
    }
    template <Numeric T> Timespan &operator/=(const T scalar)
    {
        m_Elapsed /= scalar;
        return *this;
    }

  private:
    Nanoseconds m_Elapsed;
};

KIT_NAMESPACE_END