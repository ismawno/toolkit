#pragma once

#ifndef TKIT_ENABLE_PROFILING
#    error                                                                                                             \
        "[TOOLKIT][PROFILING] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_PROFILING"
#endif

#include "tkit/preprocessor/system.hpp"
#include "tkit/utils/concepts.hpp"
#include <chrono>

namespace TKit
{
class TKIT_API Timespan
{
  public:
    using Nanoseconds = std::chrono::nanoseconds;
    using Microseconds = std::chrono::microseconds;
    using Milliseconds = std::chrono::milliseconds;
    using Seconds = std::chrono::seconds;

    Timespan(const Nanoseconds p_Elapsed) : m_Elapsed(p_Elapsed)
    {
    }

    template <typename TimeUnit, Numeric T = f32> T As() const
    {
        if constexpr (Float<T>)
            return std::chrono::duration<T, typename TimeUnit::period>(m_Elapsed).count();
        else
        {
            const auto asTimeUnit = std::chrono::duration_cast<TimeUnit>(m_Elapsed);
            return std::chrono::duration<T, typename TimeUnit::period>(asTimeUnit).count();
        }
    }

    template <Numeric T = u64> T AsNanoseconds() const
    {
        return As<Nanoseconds, T>();
    }
    template <Numeric T = f32> T AsMicroseconds() const
    {
        return As<Microseconds, T>();
    }
    template <Numeric T = f32> T AsMilliseconds() const
    {
        return As<Milliseconds, T>();
    }
    template <Numeric T = f32> T AsSeconds() const
    {
        return As<Seconds, T>();
    }

    template <typename TimeUnit, Numeric T> static Timespan From(const T p_Elapsed)
    {
        return Timespan(std::chrono::duration<T, typename TimeUnit::period>(p_Elapsed));
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

    template <Numeric T> friend Timespan operator*(const T p_Scalar, const Timespan &p_Timespan)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(p_Timespan.m_Elapsed * p_Scalar));
    }
    template <Numeric T> friend Timespan operator/(const T p_Scalar, const Timespan &p_Timespan)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(p_Scalar / p_Timespan.m_Elapsed));
    }

    template <Numeric T> friend Timespan operator*(const Timespan &p_Timespan, const T p_Scalar)
    {
        return Timespan(std::chrono::round<Timespan::Nanoseconds>(p_Timespan.m_Elapsed * p_Scalar));
    }
    template <Numeric T> friend Timespan operator/(const Timespan &p_Timespan, const T p_Scalar)
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

    template <Numeric T> Timespan &operator*=(const T p_Scalar)
    {
        m_Elapsed *= p_Scalar;
        return *this;
    }
    template <Numeric T> Timespan &operator/=(const T p_Scalar)
    {
        m_Elapsed /= p_Scalar;
        return *this;
    }

  private:
    Nanoseconds m_Elapsed;
};
} // namespace TKit
