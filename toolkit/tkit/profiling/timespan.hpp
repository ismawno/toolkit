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

    explicit Timespan(const Nanoseconds p_Elapsed = Nanoseconds::zero()) : m_Elapsed(p_Elapsed)
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

    template <typename TimeUnit, Detail::Numeric T> static Timespan From(const T p_Elapsed)
    {
        using InputDuration = std::chrono::duration<T, typename TimeUnit::period>;
        return Timespan(std::chrono::duration_cast<Nanoseconds>(InputDuration{p_Elapsed}));
    }

    template <Detail::Numeric T> static Timespan FromNanoseconds(const T p_Elapsed)
    {
        return From<Nanoseconds>(p_Elapsed);
    }
    template <Detail::Numeric T> static Timespan FromMicroseconds(const T p_Elapsed)
    {
        return From<Microseconds>(p_Elapsed);
    }
    template <Detail::Numeric T> static Timespan FromMilliseconds(const T p_Elapsed)
    {
        return From<Milliseconds>(p_Elapsed);
    }
    template <Detail::Numeric T> static Timespan FromSeconds(const T p_Elapsed)
    {
        return From<Seconds>(p_Elapsed);
    }

    static void Sleep(Timespan p_Duration);

    std::strong_ordering operator<=>(const Timespan &p_Other) const = default;

    friend Timespan operator+(const Timespan &p_Left, const Timespan &p_Right)
    {
        return Timespan(p_Left.m_Elapsed + p_Right.m_Elapsed);
    }
    friend Timespan operator-(const Timespan &p_Left, const Timespan &p_Right)
    {
        return Timespan(p_Left.m_Elapsed - p_Right.m_Elapsed);
    }

    template <Detail::Numeric T> friend Timespan operator*(const T p_Scalar, const Timespan &p_Timespan)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(p_Timespan.m_Elapsed * p_Scalar));
    }
    template <Detail::Numeric T> friend Timespan operator/(const T p_Scalar, const Timespan &p_Timespan)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(p_Scalar / p_Timespan.m_Elapsed));
    }

    template <Detail::Numeric T> friend Timespan operator*(const Timespan &p_Timespan, const T p_Scalar)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(p_Timespan.m_Elapsed * p_Scalar));
    }
    template <Detail::Numeric T> friend Timespan operator/(const Timespan &p_Timespan, const T p_Scalar)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(p_Timespan.m_Elapsed / p_Scalar));
    }

    Timespan &operator+=(const Timespan &p_Other)
    {
        m_Elapsed += p_Other.m_Elapsed;
        return *this;
    }

    Timespan &operator-=(const Timespan &p_Other)
    {
        m_Elapsed -= p_Other.m_Elapsed;
        return *this;
    }

    template <Detail::Numeric T> Timespan &operator*=(const T p_Scalar)
    {
        m_Elapsed *= p_Scalar;
        return *this;
    }
    template <Detail::Numeric T> Timespan &operator/=(const T p_Scalar)
    {
        m_Elapsed /= p_Scalar;
        return *this;
    }

  private:
    Nanoseconds m_Elapsed;
};
} // namespace TKit
