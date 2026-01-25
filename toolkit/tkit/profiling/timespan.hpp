#pragma once

#ifndef TKIT_ENABLE_PROFILING
#    error                                                                                                             \
        "[TOOLKIT][PROFILING] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_PROFILING"
#endif

#include "tkit/utils/alias.hpp"
#include "tkit/preprocessor/system.hpp"
#include <chrono>

namespace TKit::Detail
{
template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

} // namespace TKit::Detail

namespace TKit
{
class Timespan
{
  public:
    using Nanoseconds = std::chrono::nanoseconds;
    using Microseconds = std::chrono::microseconds;
    using Milliseconds = std::chrono::milliseconds;
    using Seconds = std::chrono::seconds;

    explicit Timespan(const Nanoseconds elapsed = Nanoseconds::zero()) : m_Elapsed(elapsed)
    {
    }

    template <typename TimeUnit, Detail::Numeric T = f32> T As() const
    {
        if constexpr (std::is_floating_point_v<T>)
            return std::chrono::duration<T, typename TimeUnit::period>(m_Elapsed).count();
        else
        {
            const auto asTimeUnit = std::chrono::duration_cast<TimeUnit>(m_Elapsed);
            return std::chrono::duration<T, typename TimeUnit::period>(asTimeUnit).count();
        }
    }

    template <Detail::Numeric T = u64> T AsNanoseconds() const
    {
        return As<Nanoseconds, T>();
    }
    template <Detail::Numeric T = f32> T AsMicroseconds() const
    {
        return As<Microseconds, T>();
    }
    template <Detail::Numeric T = f32> T AsMilliseconds() const
    {
        return As<Milliseconds, T>();
    }
    template <Detail::Numeric T = f32> T AsSeconds() const
    {
        return As<Seconds, T>();
    }

    template <typename TimeUnit, Detail::Numeric T> static Timespan From(const T elapsed)
    {
        using InputDuration = std::chrono::duration<T, typename TimeUnit::period>;
        return Timespan(std::chrono::duration_cast<Nanoseconds>(InputDuration{elapsed}));
    }

    template <Detail::Numeric T> static Timespan FromNanoseconds(const T elapsed)
    {
        return From<Nanoseconds>(elapsed);
    }
    template <Detail::Numeric T> static Timespan FromMicroseconds(const T elapsed)
    {
        return From<Microseconds>(elapsed);
    }
    template <Detail::Numeric T> static Timespan FromMilliseconds(const T elapsed)
    {
        return From<Milliseconds>(elapsed);
    }
    template <Detail::Numeric T> static Timespan FromSeconds(const T elapsed)
    {
        return From<Seconds>(elapsed);
    }

    static void Sleep(Timespan duration);

    std::strong_ordering operator<=>(const Timespan &other) const = default;

    friend Timespan operator+(const Timespan &left, const Timespan &right)
    {
        return Timespan(left.m_Elapsed + right.m_Elapsed);
    }
    friend Timespan operator-(const Timespan &left, const Timespan &right)
    {
        return Timespan(left.m_Elapsed - right.m_Elapsed);
    }

    template <Detail::Numeric T> friend Timespan operator*(const T scalar, const Timespan &timespan)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(timespan.m_Elapsed * scalar));
    }
    template <Detail::Numeric T> friend Timespan operator/(const T scalar, const Timespan &timespan)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(scalar / timespan.m_Elapsed));
    }

    template <Detail::Numeric T> friend Timespan operator*(const Timespan &timespan, const T scalar)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(timespan.m_Elapsed * scalar));
    }
    template <Detail::Numeric T> friend Timespan operator/(const Timespan &timespan, const T scalar)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(timespan.m_Elapsed / scalar));
    }

    Timespan &operator+=(const Timespan &other)
    {
        m_Elapsed += other.m_Elapsed;
        return *this;
    }

    Timespan &operator-=(const Timespan &other)
    {
        m_Elapsed -= other.m_Elapsed;
        return *this;
    }

    template <Detail::Numeric T> Timespan &operator*=(const T scalar)
    {
        m_Elapsed *= scalar;
        return *this;
    }
    template <Detail::Numeric T> Timespan &operator/=(const T scalar)
    {
        m_Elapsed /= scalar;
        return *this;
    }

  private:
    Nanoseconds m_Elapsed;
};
} // namespace TKit
