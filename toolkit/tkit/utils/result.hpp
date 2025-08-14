#pragma once

#include "tkit/utils/logging.hpp"
#include "tkit/container/storage.hpp"

namespace TKit
{
/**
 * @brief A result class that can hold either a value of type `T` or an error message of type ErrorType.
 *
 * This class is meant to be used in functions that can fail and return an error message, or succeed and return a value.
 * The main difference between this class and `std::optional` is that this class explicitly holds an error if the result
 * could not be computed. It is meant to make my life easier to be honest.
 *
 * Using the default constructor will create an uninitialized `Result`. Make sure to instantiate it with either `Ok` or
 * `Error` before using it.
 *
 * @tparam T The type of the value that can be held.
 * @tparam ErrorType The type of the error message that can be held.
 */
template <typename T = void, typename ErrorType = const char *> class Result
{
  public:
    /**
     * @brief Construct a `Result` object with a value of type `T`.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     */
    template <typename... ValueArgs> static Result Ok(ValueArgs &&...p_Args) noexcept
    {
        Result result{};
        result.m_Ok = true;
        result.m_Value.Construct(std::forward<ValueArgs>(p_Args)...);
        return result;
    }

    /**
     * @brief Construct a `Result` object with an error message of type `ErrorType`.
     *
     * @param p_Args The arguments to pass to the constructor of `ErrorType`.
     */
    template <typename... ErrorArgs> static Result Error(ErrorArgs &&...p_Args) noexcept
    {
        Result result{};
        result.m_Ok = false;
        result.m_Error.Construct(std::forward<ErrorArgs>(p_Args)...);
        return result;
    }

    Result(const Result &p_Other) noexcept
        requires(std::copy_constructible<T> && std::copy_constructible<ErrorType>)
        : m_Ok(p_Other.m_Ok)
    {
        if (m_Ok)
            m_Value.Construct(*p_Other.m_Value.Get());
        else
            m_Error.Construct(*p_Other.m_Error.Get());
    }
    Result(Result &&p_Other) noexcept
        requires(std::move_constructible<T> && std::move_constructible<ErrorType>)
        : m_Ok(p_Other.m_Ok)
    {
        if (m_Ok)
            m_Value.Construct(std::move(*p_Other.m_Value.Get()));
        else
            m_Error.Construct(std::move(*p_Other.m_Error.Get()));
    }

    Result &operator=(const Result &p_Other) noexcept
        requires(std::is_copy_assignable_v<T> && std::is_copy_assignable_v<ErrorType>)
    {
        if (this == &p_Other)
            return *this;
        destroy();
        m_Ok = p_Other.m_Ok;
        if (m_Ok)
            m_Value.Construct(*p_Other.m_Value.Get());
        else
            m_Error.Construct(*p_Other.m_Error.Get());

        return *this;
    }
    Result &operator=(Result &&p_Other) noexcept
        requires(std::is_move_assignable_v<T> && std::is_move_assignable_v<ErrorType>)
    {
        if (this == &p_Other)
            return *this;
        destroy();
        m_Ok = p_Other.m_Ok;
        if (m_Ok)
            m_Value.Construct(std::move(*p_Other.m_Value.Get()));
        else
            m_Error.Construct(std::move(*p_Other.m_Error.Get()));

        return *this;
    }

    ~Result() noexcept
    {
        destroy();
    }

    /**
     * @brief Check if the result is valid.
     *
     */
    bool IsOk() const noexcept
    {
        return m_Ok;
    }

    /**
     * @brief Get the value of the result.
     *
     * If the result is not valid, this will cause undefined behavior.
     *
     */
    const T &GetValue() const noexcept
    {
        TKIT_ASSERT(m_Ok, "[TOOLKIT][RESULT] Result is not Ok");
        return *m_Value.Get();
    }

    /**
     * @brief Get the value of the result.
     *
     * If the result is not valid, this will cause undefined behavior.
     *
     */
    T &GetValue() noexcept
    {
        TKIT_ASSERT(m_Ok, "[TOOLKIT][RESULT] Result is not Ok");
        return *m_Value.Get();
    }

    /**
     * @brief Get the error message of the result.
     *
     * If the result is valid, this will cause undefined behavior.
     *
     */
    const ErrorType &GetError() const noexcept
    {
        TKIT_ASSERT(!m_Ok, "[TOOLKIT][RESULT] Result is Ok");
        return *m_Error.Get();
    }

    const T *operator->() const noexcept
    {
        return &GetValue();
    }
    T *operator->() noexcept
    {
        return &GetValue();
    }

    const T &operator*() const noexcept
    {
        return GetValue();
    }
    T &operator*() noexcept
    {
        return GetValue();
    }

    explicit(false) operator bool() const noexcept
    {
        return m_Ok;
    }

  private:
    Result() noexcept = default;

    void destroy() noexcept
    {
        if (m_Ok)
            m_Value.Destruct();
        else
            m_Error.Destruct();
    }

    union {
        Storage<T> m_Value;
        Storage<ErrorType> m_Error;
    };
    bool m_Ok;
};

/**
 * @brief A specialization of `Result` that does not hold a value.
 *
 * This class is meant to be used in functions that can fail and return an error message, or succeed and return nothing.
 * The main difference between this class and `std::optional` is that this class explicitly holds an error if the result
 * could not be computed. It is meant to make my life easier to be honest.
 *
 * Using the default constructor will create an uninitialized `Result`. Make sure to instantiate it with either `Ok`
 * or `Error` before using it.
 *
 * @tparam ErrorType The type of the error message that can be held.
 */
template <typename ErrorType> class Result<void, ErrorType>
{
  public:
    /**
     * @brief Construct a `Result` object with no error.
     *
     */
    static Result Ok() noexcept
    {
        Result result{};
        result.m_Ok = true;
        return result;
    }

    /**
     * @brief Construct a `Result` object with an error message of type `ErrorType`.
     *
     * @param p_Args The arguments to pass to the constructor of `ErrorType`.
     */
    template <typename... ErrorArgs> static Result Error(ErrorArgs &&...p_Args) noexcept
    {
        Result result{};
        result.m_Ok = false;
        result.m_Error.Construct(std::forward<ErrorArgs>(p_Args)...);
        return result;
    }

    Result(const Result &p_Other) noexcept
        requires(std::copy_constructible<ErrorType>)
        : m_Ok(p_Other.m_Ok)
    {
        if (!m_Ok)
            m_Error.Construct(*p_Other.m_Error.Get());
    }
    Result(Result &&p_Other) noexcept
        requires(std::move_constructible<ErrorType>)
        : m_Ok(p_Other.m_Ok)
    {
        if (!m_Ok)
            m_Error.Construct(std::move(*p_Other.m_Error.Get()));
    }

    Result &operator=(const Result &p_Other) noexcept
        requires(std::is_copy_assignable_v<ErrorType>)
    {
        if (this == &p_Other)
            return *this;
        destroy();
        m_Ok = p_Other.m_Ok;
        if (!m_Ok)
            m_Error.Construct(*p_Other.m_Error.Get());

        return *this;
    }
    Result &operator=(Result &&p_Other) noexcept
        requires(std::is_move_assignable_v<ErrorType>)
    {
        if (this == &p_Other)
            return *this;
        destroy();
        m_Ok = p_Other.m_Ok;
        if (!m_Ok)
            m_Error.Construct(std::move(*p_Other.m_Error.Get()));

        return *this;
    }

    ~Result() noexcept
    {
        destroy();
    }

    /**
     * @brief Check if the result is valid.
     *
     */
    bool IsOk() const noexcept
    {
        return m_Ok;
    }

    /**
     * @brief Get the error message of the result.
     *
     * If the result is valid, this will cause undefined behavior.
     *
     */
    const ErrorType &GetError() const noexcept
    {
        TKIT_ASSERT(!m_Ok, "[TOOLKIT][RESULT] Result is Ok");
        return *m_Error.Get();
    }

    explicit(false) operator bool() const noexcept
    {
        return m_Ok;
    }

  private:
    Result() noexcept = default;

    void destroy() noexcept
    {
        if (!m_Ok)
            m_Error.Destruct();
    }
    Storage<ErrorType> m_Error;
    bool m_Ok;
};

} // namespace TKit
