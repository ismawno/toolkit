#pragma once

#include "tkit/core/api.hpp"

namespace TKit
{
template <typename T> class Result
{
  public:
    template <typename... Args> static Result Ok(Args &&...p_Args) noexcept
    {
        return Result<T>(std::forward<Args>(p_Args)...);
    }
    static Result Error(const char *p_Error) noexcept
    {
        return Result<T>(p_Error);
    }

    explicit(false) operator bool() const noexcept
    {
        return m_Error == nullptr;
    }
    explicit(false) operator const T &() const noexcept
    {
        return Value();
    }
    const T &Value() const noexcept
    {
        return m_Value;
    }
    const char *Error() const noexcept
    {
        return m_Error;
    }

  private:
    template <typename... Args>
        requires(!std::same_as<Result, std::decay_t<Args>> && ...)
    Result(Args &&...p_Args) noexcept : m_Value(std::forward<Args>(p_Args)...)
    {
    }
    Result(const char *p_Error) noexcept : m_Error(p_Error)
    {
    }

    T m_Value;
    const char *m_Error = nullptr;
};
} // namespace TKit