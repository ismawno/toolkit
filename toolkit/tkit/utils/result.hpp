#pragma once

#include "tkit/utils/debug.hpp"
#include "tkit/container/storage.hpp"

#define TKIT_RETURN_ON_ERROR(result, ...)                                                                              \
    if (!(result))                                                                                                     \
    {                                                                                                                  \
        __VA_ARGS__;                                                                                                   \
        return (result);                                                                                               \
    }

#define TKIT_RETURN_IF_FAILED(expression, ...)                                                                         \
    if (const auto __tkit_result = (expression); !__tkit_result)                                                       \
    {                                                                                                                  \
        __VA_ARGS__;                                                                                                   \
        return __tkit_result;                                                                                          \
    }

#define TKIT_OR_ELSE(result, fallback) (result) ? (result).GetValue() : (fallback)

namespace TKit
{
using ResultFlags = u8;
enum ResultFlagBit : ResultFlags
{
    ResultFlag_Ok = 1 << 0,
    ResultFlag_Engaged = 1 << 1,
    ResultFlag_Some = ResultFlag_Ok
};
/**
 * @brief A result class that can hold either a value of type `T` or an error of type E.
 *
 * This class is meant to be used in functions that can fail and return an error, or succeed and return a value.
 * The main difference between this class and `std::optional` is that this class explicitly holds an error if the result
 * could not be computed. It is meant to make my life easier to be honest.
 *
 * Using the default constructor will create an uninitialized `Result`. Make sure to instantiate it with either `Ok` or
 * `Error` before using it.
 *
 * @tparam T The type of the value that can be held.
 * @tparam E The type of the error that can be held.
 */
template <typename T = void, typename E = const char *> class Result
{
  public:
    constexpr Result() = default;
    /**
     * @brief Construct a `Result` object with a value of type `T`.
     *
     * @param args The arguments to pass to the constructor of `T`.
     */
    template <typename... ValueArgs> static constexpr Result Ok(ValueArgs &&...args)
    {
        Result result{};
        result.m_Flags = ResultFlag_Engaged | ResultFlag_Ok;
        result.m_Value.Construct(std::forward<ValueArgs>(args)...);
        return result;
    }

    /**
     * @brief Construct a `Result` object with an error of type `E`.
     *
     * @param args The arguments to pass to the constructor of `E`.
     */
    template <typename... ErrorArgs> static constexpr Result Error(ErrorArgs &&...args)
    {
        Result result{};
        result.m_Flags = ResultFlag_Engaged;
        result.m_Error.Construct(std::forward<ErrorArgs>(args)...);
        return result;
    }

    constexpr Result(const T &ok) : m_Flags(ResultFlag_Engaged | ResultFlag_Ok)
    {
        m_Value.Construct(ok);
    }
    constexpr Result(T &&ok) : m_Flags(ResultFlag_Engaged | ResultFlag_Ok)
    {
        m_Value.Construct(std::move(ok));
    }

    constexpr Result(const E &error) : m_Flags(ResultFlag_Engaged)
    {
        m_Error.Construct(error);
    }
    constexpr Result(E &&error) : m_Flags(ResultFlag_Engaged)
    {
        m_Error.Construct(std::move(error));
    }

    template <std::convertible_to<T> U, typename Error>
        requires(!std::same_as<E, Error>)
    constexpr Result(const Result<U, Error> &other)
        requires(std::copy_constructible<T>)
        : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlags(ResultFlag_Ok),
                    "[TOOLKIT] To copy results with different error types but same value types, "
                    "copy-from result must be a value");
        m_Value.Construct(static_cast<T>(*other.m_Value.Get()));
    }
    template <typename Type, std::convertible_to<E> Error>
        requires(!std::same_as<T, Type>)
    constexpr Result(const Result<Type, Error> &other)
        requires(std::copy_constructible<E>)
        : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlags(ResultFlag_Ok),
                    "[TOOLKIT] To copy results with different value types but same error types, "
                    "copy-from result must be an error");

        m_Error.Construct(static_cast<E>(*other.m_Error.Get()));
    }

    constexpr Result(const Result &other)
        requires(std::copy_constructible<T> && std::copy_constructible<E>)
        : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        if (checkFlags(ResultFlag_Ok))
            m_Value.Construct(*other.m_Value.Get());
        else
            m_Error.Construct(*other.m_Error.Get());
    }
    constexpr Result(Result &&other)
        requires(std::move_constructible<T> && std::move_constructible<E>)
        : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        if (checkFlags(ResultFlag_Ok))
            m_Value.Construct(std::move(*other.m_Value.Get()));
        else
            m_Error.Construct(std::move(*other.m_Error.Get()));
    }

    constexpr Result &operator=(const T &ok)
    {
        destroy();
        m_Flags = ResultFlag_Engaged | ResultFlag_Ok;
        m_Value.Construct(ok);
        return *this;
    }
    constexpr Result &operator=(T &&ok)
    {
        destroy();
        m_Flags = ResultFlag_Engaged | ResultFlag_Ok;
        m_Value.Construct(std::move(ok));
        return *this;
    }

    constexpr Result &operator=(const E &error)
    {
        destroy();
        m_Flags = ResultFlag_Engaged;
        m_Error.Construct(error);
        return *this;
    }
    constexpr Result &operator=(E &&error)
    {
        destroy();
        m_Flags = ResultFlag_Engaged;
        m_Error.Construct(error);
        return *this;
    }

    template <std::convertible_to<T> U, typename Error>
        requires(!std::same_as<E, Error>)
    constexpr Result &operator=(const Result<U, Error> &other)
        requires(std::copy_constructible<T>)
    {
        destroy();
        m_Flags = other.m_Flags;
        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlags(ResultFlag_Ok),
                    "[TOOLKIT] To copy results with different error types but same value types, "
                    "copy-from result must be a value");
        m_Value.Construct(static_cast<T>(*other.m_Value.Get()));
        return *this;
    }
    template <typename Type, std::convertible_to<E> Error>
        requires(!std::same_as<T, Type>)
    constexpr Result &operator=(const Result<Type, Error> &other)
        requires(std::copy_constructible<E>)

    {
        destroy();
        m_Flags = other.m_Flags;
        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlags(ResultFlag_Ok),
                    "[TOOLKIT] To copy results with different value types but same error types, "
                    "copy-from result must be an error");

        m_Error.Construct(static_cast<E>(*other.m_Error.Get()));
        return *this;
    }

    constexpr Result &operator=(const Result &other)
    {
        if (this == &other)
            return *this;
        destroy();
        m_Flags = other.m_Flags;

        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot assign from an undefined result");
        if (checkFlags(ResultFlag_Ok))
            m_Value.Construct(*other.m_Value.Get());
        else
            m_Error.Construct(*other.m_Error.Get());

        return *this;
    }
    constexpr Result &operator=(Result &&other)
    {
        if (this == &other)
            return *this;

        destroy();
        m_Flags = other.m_Flags;

        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot assign from an undefined result");
        if (checkFlags(ResultFlag_Ok))
            m_Value.Construct(std::move(*other.m_Value.Get()));
        else
            m_Error.Construct(std::move(*other.m_Error.Get()));

        return *this;
    }

    constexpr ~Result()
    {
        destroy();
    }

    constexpr bool IsOk() const
    {
        return m_Flags == (ResultFlag_Engaged | ResultFlag_Ok);
    }
    constexpr bool IsError() const
    {
        return m_Flags == ResultFlag_Engaged;
    }
    constexpr bool IsEngaged() const
    {
        return m_Flags == ResultFlag_Engaged;
    }

    /**
     * @brief Get the value of the result.
     *
     * If the result is not valid, this will cause undefined behavior.
     *
     */
    const T &GetValue() const
    {
        TKIT_ASSERT(IsOk(), "[TOOLKIT][RESULT] Result is not Ok");
        return *m_Value.Get();
    }

    /**
     * @brief Get the value of the result.
     *
     * If the result is not valid, this will cause undefined behavior.
     *
     */
    constexpr T &GetValue()
    {
        TKIT_ASSERT(IsOk(), "[TOOLKIT][RESULT] Result is not Ok");
        return *m_Value.Get();
    }

    /**
     * @brief Get the error of the result.
     *
     * If the result is valid, this will cause undefined behavior.
     *
     */
    constexpr const E &GetError() const
    {
        TKIT_ASSERT(IsError(), "[TOOLKIT][RESULT] Result is not an error");
        return *m_Error.Get();
    }

    constexpr const T *operator->() const
    {
        return &GetValue();
    }
    constexpr T *operator->()
    {
        return &GetValue();
    }

    constexpr const T &operator*() const
    {
        return GetValue();
    }
    constexpr T &operator*()
    {
        return GetValue();
    }

    constexpr operator bool() const
    {
        return IsOk();
    }

  private:
    constexpr ResultFlags checkFlags(const ResultFlags flags) const
    {
        return m_Flags & flags;
    }
    constexpr void destroy()
    {
        if (!checkFlags(ResultFlag_Engaged))
            return;

        if (checkFlags(ResultFlag_Ok))
            m_Value.Destruct();
        else
            m_Error.Destruct();
    }

    union {
        Storage<T> m_Value;
        Storage<E> m_Error;
    };
    ResultFlags m_Flags = 0;

    template <typename Type, typename Error> friend class Result;
};

/**
 * @brief A specialization of `Result` that does not hold a value.
 *
 * This class is meant to be used in functions that can fail and return an error, or succeed and return nothing.
 * The main difference between this class and `std::optional` is that this class explicitly holds an error if the result
 * could not be computed. It is meant to make my life easier to be honest.
 *
 * Using the default constructor will create an uninitialized `Result`. Make sure to instantiate it with either `Ok`
 * or `Error` before using it.
 *
 * @tparam E The type of the error that can be held.
 */
template <typename E> class Result<void, E>
{
    Result() = default;

  public:
    /**
     * @brief Construct a `Result` object with no error.
     *
     */
    static constexpr Result Ok()
    {
        Result result{};
        result.m_Flags = ResultFlag_Engaged | ResultFlag_Ok;
        return result;
    }

    /**
     * @brief Construct a `Result` object with an error of type `E`.
     *
     * @param args The arguments to pass to the constructor of `E`.
     */
    template <typename... ErrorArgs> static constexpr Result Error(ErrorArgs &&...args)
    {
        Result result{};
        result.m_Flags = ResultFlag_Engaged;
        result.m_Error.Construct(std::forward<ErrorArgs>(args)...);
        return result;
    }

    constexpr Result(const E &error) : m_Flags(ResultFlag_Engaged)
    {
        m_Error.Construct(error);
    }
    constexpr Result(E &&error) : m_Flags(ResultFlag_Engaged)
    {
        m_Error.Construct(std::move(error));
    }

    template <typename Error>
        requires(!std::same_as<E, Error>)
    constexpr Result(const Result<void, Error> &other) : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlags(ResultFlag_Ok),
                    "[TOOLKIT] To copy results with different error types but same value types, "
                    "copy-from result must be a value");
    }
    template <typename Type, std::convertible_to<E> Error>
        requires(!std::same_as<void, Type>)
    constexpr Result(const Result<Type, Error> &other)
        requires(std::copy_constructible<E>)
        : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlags(ResultFlag_Ok),
                    "[TOOLKIT] To copy results with different value types but same error types, "
                    "copy-from result must be an error");
        m_Error.Construct(static_cast<E>(*other.m_Error.Get()));
    }

    constexpr Result(const Result &other)
        requires(std::copy_constructible<E>)
        : m_Flags(other.m_Flags)
    {
        if (IsError())
            m_Error.Construct(*other.m_Error.Get());
    }
    constexpr Result(Result &&other)
        requires(std::move_constructible<E>)
        : m_Flags(other.m_Flags)
    {
        if (IsError())
            m_Error.Construct(std::move(*other.m_Error.Get()));
    }

    constexpr Result &operator=(const E &error)
    {
        destroy();
        m_Flags = ResultFlag_Engaged;
        m_Error.Construct(error);
        return *this;
    }
    constexpr Result &operator=(E &&error)
    {
        destroy();
        m_Flags = ResultFlag_Engaged;
        m_Error.Construct(error);
        return *this;
    }

    template <typename Error>
        requires(!std::same_as<E, Error>)
    constexpr Result &operator=(const Result<void, Error> &other)
    {
        destroy();
        m_Flags = other.m_Flags;
        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlags(ResultFlag_Ok),
                    "[TOOLKIT] To copy results with different error types but same value types, "
                    "copy-from result must be a value");
        return *this;
    }
    template <typename Type, std::convertible_to<E> Error>
        requires(!std::same_as<void, Type>)
    constexpr Result &operator=(const Result<Type, Error> &other)
        requires(std::copy_constructible<E>)

    {
        destroy();
        m_Flags = other.m_Flags;
        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlags(ResultFlag_Ok),
                    "[TOOLKIT] To copy results with different value types but same error types, "
                    "copy-from result must be an error");
        m_Error.Construct(static_cast<E>(*other.m_Error.Get()));
        return *this;
    }

    constexpr Result &operator=(const Result &other)
    {
        if (this == &other)
            return *this;
        destroy();
        m_Flags = other.m_Flags;
        if (IsError())
            m_Error.Construct(*other.m_Error.Get());

        return *this;
    }
    constexpr Result &operator=(Result &&other)
    {
        if (this == &other)
            return *this;
        destroy();
        m_Flags = other.m_Flags;
        if (IsError())
            m_Error.Construct(std::move(*other.m_Error.Get()));

        return *this;
    }

    constexpr ~Result()
    {
        destroy();
    }

    constexpr bool IsOk() const
    {
        return m_Flags == (ResultFlag_Engaged | ResultFlag_Ok);
    }
    constexpr bool IsError() const
    {
        return m_Flags == ResultFlag_Engaged;
    }
    constexpr bool IsEngaged() const
    {
        return m_Flags == ResultFlag_Engaged;
    }

    /**
     * @brief Get the error of the result.
     *
     * If the result is valid, this will cause undefined behavior.
     *
     */
    constexpr const E &GetError() const
    {
        TKIT_ASSERT(IsError(), "[TOOLKIT][RESULT] Result is not an error");
        return *m_Error.Get();
    }

    constexpr operator bool() const
    {
        return IsOk();
    }

  private:
    constexpr ResultFlags checkFlags(const ResultFlags flags) const
    {
        return m_Flags & flags;
    }
    constexpr void destroy()
    {
        if (IsError())
            m_Error.Destruct();
    }
    Storage<E> m_Error;
    ResultFlags m_Flags = 0;

    template <typename Type, typename Error> friend class Result;
};

/**
 * @brief A specialization of `Result` that does not hold an error.
 *
 * This class is an implementation of an `Optional<T>` type essentially.
 *
 * Using the default constructor will create an uninitialized `Result`. Make sure to instantiate it with either `Ok`
 * or `Error` before using it.
 *
 * @tparam E The type of the error that can be held.
 */
template <typename T> class Result<T, void>
{
  public:
    Result() = default;

    template <typename... ValueArgs> static constexpr Result Some(ValueArgs &&...args)
    {
        Result result{};
        result.m_Flags = ResultFlag_Engaged | ResultFlag_Some;
        result.m_Value.Construct(std::forward<ValueArgs>(args)...);
    }
    static constexpr Result None()
    {
        Result result{};
        result.m_Flags = ResultFlag_Engaged;
        return result;
    }

    constexpr Result(const T &value) : m_Flags(ResultFlag_Engaged | ResultFlag_Some)
    {
        m_Value.Construct(value);
    }
    constexpr Result(T &&value) : m_Flags(ResultFlag_Engaged | ResultFlag_Some)
    {
        m_Value.Construct(std::move(value));
    }

    template <std::convertible_to<T> U, typename Error>
        requires(!std::same_as<void, Error>)
    constexpr Result(const Result<U, Error> &other)
        requires(std::copy_constructible<T>)
        : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlags(ResultFlag_Some),
                    "[TOOLKIT] To copy results with different error types but same value types, "
                    "copy-from result must be a value");

        m_Value.Construct(static_cast<T>(*other.m_Value.Get()));
    }
    template <typename Type>
        requires(!std::same_as<T, Type>)
    constexpr Result(const Result<Type, void> &other) : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlags(ResultFlag_Some),
                    "[TOOLKIT] To copy results with different value types but same error types, "
                    "copy-from result must be an error");
    }

    constexpr Result(const Result &other)
        requires(std::copy_constructible<T>)
        : m_Flags(other.m_Flags)
    {
        if (IsSome())
            m_Value.Construct(*other.m_Value.Get());
    }
    constexpr Result(Result &&other)
        requires(std::move_constructible<T>)
        : m_Flags(other.m_Flags)
    {
        if (IsSome())
            m_Value.Construct(std::move(*other.m_Value.Get()));
    }

    constexpr Result &operator=(const T &value)
    {
        destroy();
        m_Flags = ResultFlag_Engaged;
        m_Value.Construct(value);
        return *this;
    }
    constexpr Result &operator=(T &&value)
    {
        destroy();
        m_Flags = ResultFlag_Engaged;
        m_Value.Construct(value);
        return *this;
    }

    template <std::convertible_to<T> U, typename Error>
        requires(!std::same_as<void, Error>)
    constexpr Result &operator=(const Result<U, Error> &other)
        requires(std::copy_constructible<T>)
    {
        destroy();
        m_Flags = other.m_Flags;
        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlags(ResultFlag_Some),
                    "[TOOLKIT] To copy results with different error types but same value types, "
                    "copy-from result must be a value");
        m_Value.Construct(static_cast<T>(*other.m_Value.Get()));
        return *this;
    }
    template <typename Type>
        requires(!std::same_as<T, Type>)
    constexpr Result &operator=(const Result<Type, void> &other)

    {
        destroy();
        m_Flags = other.m_Flags;
        TKIT_ASSERT(checkFlags(ResultFlag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlags(ResultFlag_Some),
                    "[TOOLKIT] To copy results with different value types but same error types, "
                    "copy-from result must be an error");
        return *this;
    }

    constexpr Result &operator=(const Result &other)
    {
        if (this == &other)
            return *this;
        destroy();
        m_Flags = other.m_Flags;
        if (IsSome())
            m_Value.Construct(*other.m_Value.Get());

        return *this;
    }
    constexpr Result &operator=(Result &&other)
    {
        if (this == &other)
            return *this;
        destroy();
        m_Flags = other.m_Flags;
        if (IsSome())
            m_Value.Construct(std::move(*other.m_Value.Get()));

        return *this;
    }

    constexpr ~Result()
    {
        destroy();
    }

    constexpr bool IsSome() const
    {
        return m_Flags == (ResultFlag_Engaged | ResultFlag_Some);
    }
    constexpr bool IsEngaged() const
    {
        return m_Flags == ResultFlag_Engaged;
    }

    constexpr const T &GetValue() const
    {
        TKIT_ASSERT(IsSome(), "[TOOLKIT][RESULT] Result is not an error");
        return *m_Value.Get();
    }
    constexpr T &GetValue()
    {
        TKIT_ASSERT(IsSome(), "[TOOLKIT][RESULT] Result is not an error");
        return *m_Value.Get();
    }

    constexpr operator bool() const
    {
        return IsSome();
    }
    constexpr const T *operator->() const
    {
        return &GetValue();
    }
    constexpr T *operator->()
    {
        return &GetValue();
    }

    constexpr const T &operator*() const
    {
        return GetValue();
    }
    constexpr T &operator*()
    {
        return GetValue();
    }

  private:
    constexpr ResultFlags checkFlags(const ResultFlags flags) const
    {
        return m_Flags & flags;
    }
    constexpr void destroy()
    {
        if (IsSome())
            m_Value.Destruct();
    }
    Storage<T> m_Value;
    ResultFlags m_Flags = 0;

    template <typename Type, typename Error> friend class Result;
};

} // namespace TKit
