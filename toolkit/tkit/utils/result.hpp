#pragma once

#include "tkit/utils/debug.hpp"
#include "tkit/container/storage.hpp"

#define TKIT_RETURN_ON_ERROR(result)                                                                                 \
    if (!result)                                                                                                     \
    return result

#define TKIT_RETURN_IF_FAILED(expression)                                                                            \
    if (const auto __tkit_result = expression; !__tkit_result)                                                       \
        return __tkit_result;

#define TKIT_OR_ELSE(result, fallback) result ? result.GetValue() : fallback

namespace TKit
{
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
    using Flags = u8;
    enum FlagBits : Flags
    {
        Flag_Ok = 1 << 0,
        Flag_Engaged = 1 << 1
    };

  public:
    /**
     * @brief Construct a `Result` object with a value of type `T`.
     *
     * @param args The arguments to pass to the constructor of `T`.
     */
    template <typename... ValueArgs> static Result Ok(ValueArgs &&...args)
    {
        Result result{};
        result.m_Flags = Flag_Engaged | Flag_Ok;
        result.m_Value.Construct(std::forward<ValueArgs>(args)...);
        return result;
    }

    /**
     * @brief Construct a `Result` object with an error of type `E`.
     *
     * @param args The arguments to pass to the constructor of `E`.
     */
    template <typename... ErrorArgs> static Result Error(ErrorArgs &&...args)
    {
        Result result{};
        result.m_Flags = Flag_Engaged;
        result.m_Error.Construct(std::forward<ErrorArgs>(args)...);
        return result;
    }

    Result(const T &ok) : m_Flags(Flag_Engaged | Flag_Ok)
    {
        m_Value.Construct(ok);
    }
    Result(T &&ok) : m_Flags(Flag_Engaged | Flag_Ok)
    {
        m_Value.Construct(std::move(ok));
    }

    Result(const E &error) : m_Flags(Flag_Engaged)
    {
        m_Error.Construct(error);
    }
    Result(E &&error) : m_Flags(Flag_Engaged)
    {
        m_Error.Construct(std::move(error));
    }

    template <std::convertible_to<T> U, typename Error>
        requires(!std::same_as<E, Error>)
    Result(const Result<U, Error> &other)
        requires(std::copy_constructible<T>)
        : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlags(Flag_Ok), "[TOOLKIT] To copy results with different error types but same value types, "
                                         "copy-from result must be a value");
        m_Value.Construct(static_cast<T>(*other.m_Value.Get()));
    }
    template <typename Type, std::convertible_to<E> Error>
        requires(!std::same_as<T, Type>)
    Result(const Result<Type, Error> &other)
        requires(std::copy_constructible<E>)
        : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlags(Flag_Ok), "[TOOLKIT] To copy results with different value types but same error types, "
                                          "copy-from result must be an error");

        m_Error.Construct(static_cast<E>(*other.m_Error.Get()));
    }

    Result(const Result &other)
        requires(std::copy_constructible<T> && std::copy_constructible<E>)
        : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        if (checkFlags(Flag_Ok))
            m_Value.Construct(*other.m_Value.Get());
        else
            m_Error.Construct(*other.m_Error.Get());
    }
    Result(Result &&other)
        requires(std::move_constructible<T> && std::move_constructible<E>)
        : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        if (checkFlags(Flag_Ok))
            m_Value.Construct(std::move(*other.m_Value.Get()));
        else
            m_Error.Construct(std::move(*other.m_Error.Get()));
    }

    Result &operator=(const T &ok)
    {
        destroy();
        m_Flags = Flag_Engaged | Flag_Ok;
        m_Value.Construct(ok);
        return *this;
    }
    Result &operator=(T &&ok)
    {
        destroy();
        m_Flags = Flag_Engaged | Flag_Ok;
        m_Value.Construct(std::move(ok));
        return *this;
    }

    Result &operator=(const E &error)
    {
        destroy();
        m_Flags = Flag_Engaged;
        m_Error.Construct(error);
        return *this;
    }
    Result &operator=(E &&error)
    {
        destroy();
        m_Flags = Flag_Engaged;
        m_Error.Construct(error);
        return *this;
    }

    template <std::convertible_to<T> U, typename Error>
        requires(!std::same_as<E, Error>)
    Result &operator=(const Result<U, Error> &other)
        requires(std::copy_constructible<T>)
    {
        destroy();
        m_Flags = other.m_Flags;
        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlags(Flag_Ok), "[TOOLKIT] To copy results with different error types but same value types, "
                                         "copy-from result must be a value");
        m_Value.Construct(static_cast<T>(*other.m_Value.Get()));
        return *this;
    }
    template <typename Type, std::convertible_to<E> Error>
        requires(!std::same_as<T, Type>)
    Result &operator=(const Result<Type, Error> &other)
        requires(std::copy_constructible<E>)

    {
        destroy();
        m_Flags = other.m_Flags;
        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlags(Flag_Ok), "[TOOLKIT] To copy results with different value types but same error types, "
                                          "copy-from result must be an error");

        m_Error.Construct(static_cast<E>(*other.m_Error.Get()));
        return *this;
    }

    Result &operator=(const Result &other)
    {
        if (this == &other)
            return *this;
        destroy();
        m_Flags = other.m_Flags;

        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot assign from an undefined result");
        if (checkFlags(Flag_Ok))
            m_Value.Construct(*other.m_Value.Get());
        else
            m_Error.Construct(*other.m_Error.Get());

        return *this;
    }
    Result &operator=(Result &&other)
    {
        if (this == &other)
            return *this;

        destroy();
        m_Flags = other.m_Flags;

        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot assign from an undefined result");
        if (checkFlags(Flag_Ok))
            m_Value.Construct(std::move(*other.m_Value.Get()));
        else
            m_Error.Construct(std::move(*other.m_Error.Get()));

        return *this;
    }

    ~Result()
    {
        destroy();
    }

    /**
     * @brief Check if the result is valid.
     *
     */
    bool IsOk() const
    {
        return checkFlags(Flag_Engaged) && checkFlags(Flag_Ok);
    }

    bool IsError() const
    {
        return checkFlags(Flag_Engaged) && !checkFlags(Flag_Ok);
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
    T &GetValue()
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
    const E &GetError() const
    {
        TKIT_ASSERT(IsError(), "[TOOLKIT][RESULT] Result is not an error");
        return *m_Error.Get();
    }

    const T *operator->() const
    {
        return &GetValue();
    }
    T *operator->()
    {
        return &GetValue();
    }

    const T &operator*() const
    {
        return GetValue();
    }
    T &operator*()
    {
        return GetValue();
    }

    operator bool() const
    {
        return IsOk();
    }

  private:
    Result() = default;

    bool checkFlags(const Flags flags) const
    {
        return m_Flags & flags;
    }
    void destroy()
    {
        if (!checkFlags(Flag_Engaged))
            return;

        if (checkFlags(Flag_Ok))
            m_Value.Destruct();
        else
            m_Error.Destruct();
    }

    union {
        Storage<T> m_Value;
        Storage<E> m_Error;
    };
    Flags m_Flags = 0;

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
    using Flags = u8;
    enum FlagBits : Flags
    {
        Flag_Ok = 1 << 0,
        Flag_Engaged = 1 << 1
    };

  public:
    /**
     * @brief Construct a `Result` object with no error.
     *
     */
    static Result Ok()
    {
        Result result{};
        result.m_Flags = Flag_Engaged | Flag_Ok;
        return result;
    }

    /**
     * @brief Construct a `Result` object with an error of type `E`.
     *
     * @param args The arguments to pass to the constructor of `E`.
     */
    template <typename... ErrorArgs> static Result Error(ErrorArgs &&...args)
    {
        Result result{};
        result.m_Flags = Flag_Engaged;
        result.m_Error.Construct(std::forward<ErrorArgs>(args)...);
        return result;
    }

    Result(const E &error) : m_Flags(Flag_Engaged)
    {
        m_Error.Construct(error);
    }
    Result(E &&error) : m_Flags(Flag_Engaged)
    {
        m_Error.Construct(std::move(error));
    }

    template <typename Error>
        requires(!std::same_as<E, Error>)
    Result(const Result<void, Error> &other) : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlags(Flag_Ok), "[TOOLKIT] To copy results with different error types but same value types, "
                                         "copy-from result must be a value");
    }
    template <typename Type, std::convertible_to<E> Error>
        requires(!std::same_as<void, Type>)
    Result(const Result<Type, Error> &other)
        requires(std::copy_constructible<E>)
        : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlags(Flag_Ok), "[TOOLKIT] To copy results with different value types but same error types, "
                                          "copy-from result must be an error");
        m_Error.Construct(static_cast<E>(*other.m_Error.Get()));
    }

    Result(const Result &other)
        requires(std::copy_constructible<E>)
        : m_Flags(other.m_Flags)
    {
        if (IsError())
            m_Error.Construct(*other.m_Error.Get());
    }
    Result(Result &&other)
        requires(std::move_constructible<E>)
        : m_Flags(other.m_Flags)
    {
        if (IsError())
            m_Error.Construct(std::move(*other.m_Error.Get()));
    }

    Result &operator=(const E &error)
    {
        destroy();
        m_Flags = Flag_Engaged;
        m_Error.Construct(error);
        return *this;
    }
    Result &operator=(E &&error)
    {
        destroy();
        m_Flags = Flag_Engaged;
        m_Error.Construct(error);
        return *this;
    }

    template <typename Error>
        requires(!std::same_as<E, Error>)
    Result &operator=(const Result<void, Error> &other)
    {
        destroy();
        m_Flags = other.m_Flags;
        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlags(Flag_Ok), "[TOOLKIT] To copy results with different error types but same value types, "
                                         "copy-from result must be a value");
        return *this;
    }
    template <typename Type, std::convertible_to<E> Error>
        requires(!std::same_as<void, Type>)
    Result &operator=(const Result<Type, Error> &other)
        requires(std::copy_constructible<E>)

    {
        destroy();
        m_Flags = other.m_Flags;
        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlags(Flag_Ok), "[TOOLKIT] To copy results with different value types but same error types, "
                                          "copy-from result must be an error");
        m_Error.Construct(static_cast<E>(*other.m_Error.Get()));
        return *this;
    }

    Result &operator=(const Result &other)
    {
        if (this == &other)
            return *this;
        destroy();
        m_Flags = other.m_Flags;
        if (IsError())
            m_Error.Construct(*other.m_Error.Get());

        return *this;
    }
    Result &operator=(Result &&other)
    {
        if (this == &other)
            return *this;
        destroy();
        m_Flags = other.m_Flags;
        if (IsError())
            m_Error.Construct(std::move(*other.m_Error.Get()));

        return *this;
    }

    ~Result()
    {
        destroy();
    }

    /**
     * @brief Check if the result is valid.
     *
     */
    bool IsOk() const
    {
        return checkFlags(Flag_Engaged) && checkFlags(Flag_Ok);
    }

    bool IsError() const
    {
        return checkFlags(Flag_Engaged) && !checkFlags(Flag_Ok);
    }

    /**
     * @brief Get the error of the result.
     *
     * If the result is valid, this will cause undefined behavior.
     *
     */
    const E &GetError() const
    {
        TKIT_ASSERT(IsError(), "[TOOLKIT][RESULT] Result is not an error");
        return *m_Error.Get();
    }

    operator bool() const
    {
        return IsOk();
    }

  private:
    Result() = default;

    bool checkFlags(const FlagBits flag) const
    {
        return m_Flags & flag;
    }
    void destroy()
    {
        if (IsError())
            m_Error.Destruct();
    }
    Storage<E> m_Error;
    Flags m_Flags = 0;

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
    using Flags = u8;
    enum FlagBits : Flags
    {
        Flag_Some = 1 << 0,
        Flag_Engaged = 1 << 1
    };

  public:
    template <typename... ValueArgs> static Result Some(ValueArgs &&...args)
    {
        Result result{};
        result.m_Flags = Flag_Engaged | Flag_Some;
        result.m_Value.Construct(std::forward<ValueArgs>(args)...);
    }
    static Result None()
    {
        Result result{};
        result.m_Flags = Flag_Engaged;
        return result;
    }

    Result(const T &value) : m_Flags(Flag_Engaged | Flag_Some)
    {
        m_Value.Construct(value);
    }
    Result(T &&value) : m_Flags(Flag_Engaged | Flag_Some)
    {
        m_Value.Construct(std::move(value));
    }

    template <std::convertible_to<T> U, typename Error>
        requires(!std::same_as<void, Error>)
    Result(const Result<U, Error> &other)
        requires(std::copy_constructible<T>)
        : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlags(Flag_Some), "[TOOLKIT] To copy results with different error types but same value types, "
                                           "copy-from result must be a value");

        m_Value.Construct(static_cast<T>(*other.m_Value.Get()));
    }
    template <typename Type>
        requires(!std::same_as<T, Type>)
    Result(const Result<Type, void> &other) : m_Flags(other.m_Flags)
    {
        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlags(Flag_Some),
                    "[TOOLKIT] To copy results with different value types but same error types, "
                    "copy-from result must be an error");
    }

    Result(const Result &other)
        requires(std::copy_constructible<T>)
        : m_Flags(other.m_Flags)
    {
        if (IsSome())
            m_Value.Construct(*other.m_Value.Get());
    }
    Result(Result &&other)
        requires(std::move_constructible<T>)
        : m_Flags(other.m_Flags)
    {
        if (IsSome())
            m_Value.Construct(std::move(*other.m_Value.Get()));
    }

    Result &operator=(const T &value)
    {
        destroy();
        m_Flags = Flag_Engaged;
        m_Value.Construct(value);
        return *this;
    }
    Result &operator=(T &&value)
    {
        destroy();
        m_Flags = Flag_Engaged;
        m_Value.Construct(value);
        return *this;
    }

    template <std::convertible_to<T> U, typename Error>
        requires(!std::same_as<void, Error>)
    Result &operator=(const Result<U, Error> &other)
        requires(std::copy_constructible<T>)
    {
        destroy();
        m_Flags = other.m_Flags;
        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlags(Flag_Some), "[TOOLKIT] To copy results with different error types but same value types, "
                                           "copy-from result must be a value");
        m_Value.Construct(static_cast<T>(*other.m_Value.Get()));
        return *this;
    }
    template <typename Type>
        requires(!std::same_as<T, Type>)
    Result &operator=(const Result<Type, void> &other)

    {
        destroy();
        m_Flags = other.m_Flags;
        TKIT_ASSERT(checkFlags(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlags(Flag_Some),
                    "[TOOLKIT] To copy results with different value types but same error types, "
                    "copy-from result must be an error");
        return *this;
    }

    Result &operator=(const Result &other)
    {
        if (this == &other)
            return *this;
        destroy();
        m_Flags = other.m_Flags;
        if (IsSome())
            m_Value.Construct(*other.m_Value.Get());

        return *this;
    }
    Result &operator=(Result &&other)
    {
        if (this == &other)
            return *this;
        destroy();
        m_Flags = other.m_Flags;
        if (IsSome())
            m_Value.Construct(std::move(*other.m_Value.Get()));

        return *this;
    }

    ~Result()
    {
        destroy();
    }

    bool IsSome() const
    {
        return checkFlags(Flag_Engaged) && checkFlags(Flag_Some);
    }

    /**
     * @brief Get the value of the result.
     *
     * If the result is not valid, this will cause undefined behavior.
     *
     */
    const T &GetValue() const
    {
        TKIT_ASSERT(IsSome(), "[TOOLKIT][RESULT] Result is not an error");
        return *m_Value.Get();
    }

    operator bool() const
    {
        return IsSome();
    }

  private:
    Result() = default;

    bool checkFlags(const FlagBits flag) const
    {
        return m_Flags & flag;
    }
    void destroy()
    {
        if (IsSome())
            m_Value.Destruct();
    }
    Storage<T> m_Value;
    Flags m_Flags = 0;

    template <typename Type, typename Error> friend class Result;
};

} // namespace TKit
