#pragma once

#include "tkit/utils/debug.hpp"
#include "tkit/container/storage.hpp"

#define TKIT_RETURN_ON_ERROR(p_Result)                                                                                 \
    if (!p_Result)                                                                                                     \
    return p_Result

#define TKIT_OR_ELSE(p_Result, p_Fallback) p_Result ? p_Result.GetValue() : p_Fallback

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
     * @param p_Args The arguments to pass to the constructor of `T`.
     */
    template <typename... ValueArgs> static Result Ok(ValueArgs &&...p_Args)
    {
        Result result{};
        result.m_Flags = Flag_Engaged | Flag_Ok;
        result.m_Value.Construct(std::forward<ValueArgs>(p_Args)...);
        return result;
    }

    /**
     * @brief Construct a `Result` object with an error of type `E`.
     *
     * @param p_Args The arguments to pass to the constructor of `E`.
     */
    template <typename... ErrorArgs> static Result Error(ErrorArgs &&...p_Args)
    {
        Result result{};
        result.m_Flags = Flag_Engaged;
        result.m_Error.Construct(std::forward<ErrorArgs>(p_Args)...);
        return result;
    }

    Result(const T &p_Ok) : m_Flags(Flag_Engaged | Flag_Ok)
    {
        m_Value.Construct(p_Ok);
    }
    Result(T &&p_Ok) : m_Flags(Flag_Engaged | Flag_Ok)
    {
        m_Value.Construct(std::move(p_Ok));
    }

    Result(const E &p_Error) : m_Flags(Flag_Engaged)
    {
        m_Error.Construct(p_Error);
    }
    Result(E &&p_Error) : m_Flags(Flag_Engaged)
    {
        m_Error.Construct(std::move(p_Error));
    }

    template <typename Error>
        requires(!std::same_as<E, Error>)
    Result(const Result<T, Error> &p_Other)
        requires(std::copy_constructible<T>)
        : m_Flags(p_Other.m_Flags)
    {
        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlag(Flag_Ok), "[TOOLKIT] To copy results with different error types but same value types, "
                                        "copy-from result must be a value");
        m_Value.Construct(*p_Other.m_Value.Get());
    }
    template <typename Type>
        requires(!std::same_as<T, Type>)
    Result(const Result<Type, E> &p_Other)
        requires(std::copy_constructible<E>)
        : m_Flags(p_Other.m_Flags)
    {
        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlag(Flag_Ok), "[TOOLKIT] To copy results with different value types but same error types, "
                                         "copy-from result must be an error");

        m_Error.Construct(*p_Other.m_Error.Get());
    }

    Result(const Result &p_Other)
        requires(std::copy_constructible<T> && std::copy_constructible<E>)
        : m_Flags(p_Other.m_Flags)
    {
        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        if (checkFlag(Flag_Ok))
            m_Value.Construct(*p_Other.m_Value.Get());
        else
            m_Error.Construct(*p_Other.m_Error.Get());
    }
    Result(Result &&p_Other)
        requires(std::move_constructible<T> && std::move_constructible<E>)
        : m_Flags(p_Other.m_Flags)
    {
        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        if (checkFlag(Flag_Ok))
            m_Value.Construct(std::move(*p_Other.m_Value.Get()));
        else
            m_Error.Construct(std::move(*p_Other.m_Error.Get()));
    }

    Result &operator=(const T &p_Ok)
    {
        destroy();
        m_Flags = Flag_Engaged | Flag_Ok;
        m_Value.Construct(p_Ok);
        return *this;
    }
    Result &operator=(T &&p_Ok)
    {
        destroy();
        m_Flags = Flag_Engaged | Flag_Ok;
        m_Value.Construct(std::move(p_Ok));
        return *this;
    }

    Result &operator=(const E &p_Error)
    {
        destroy();
        m_Flags = Flag_Engaged;
        m_Error.Construct(p_Error);
        return *this;
    }
    Result &operator=(E &&p_Error)
    {
        destroy();
        m_Flags = Flag_Engaged;
        m_Error.Construct(p_Error);
        return *this;
    }

    template <typename Error>
        requires(!std::same_as<E, Error>)
    Result &operator=(const Result<T, Error> &p_Other)
        requires(std::copy_constructible<T>)
    {
        destroy();
        m_Flags = p_Other.m_Flags;
        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlag(Flag_Ok), "[TOOLKIT] To copy results with different error types but same value types, "
                                        "copy-from result must be a value");
        m_Value.Construct(*p_Other.m_Value.Get());
        return *this;
    }
    template <typename Type>
        requires(!std::same_as<T, Type>)
    Result &operator=(const Result<Type, E> &p_Other)
        requires(std::copy_constructible<E>)

    {
        destroy();
        m_Flags = p_Other.m_Flags;
        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlag(Flag_Ok), "[TOOLKIT] To copy results with different value types but same error types, "
                                         "copy-from result must be an error");

        m_Error.Construct(*p_Other.m_Error.Get());
        return *this;
    }

    Result &operator=(const Result &p_Other)
        requires(std::is_copy_assignable_v<T> && std::is_copy_assignable_v<E>)
    {
        if (this == &p_Other)
            return *this;
        destroy();
        m_Flags = p_Other.m_Flags;

        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot assign from an undefined result");
        if (checkFlag(Flag_Ok))
            m_Value.Construct(*p_Other.m_Value.Get());
        else
            m_Error.Construct(*p_Other.m_Error.Get());

        return *this;
    }
    Result &operator=(Result &&p_Other)
        requires(std::is_move_assignable_v<T> && std::is_move_assignable_v<E>)
    {
        if (this == &p_Other)
            return *this;

        destroy();
        m_Flags = p_Other.m_Flags;

        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot assign from an undefined result");
        if (checkFlag(Flag_Ok))
            m_Value.Construct(std::move(*p_Other.m_Value.Get()));
        else
            m_Error.Construct(std::move(*p_Other.m_Error.Get()));

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
        return checkFlag(Flag_Engaged) && checkFlag(Flag_Ok);
    }

    bool IsError() const
    {
        return checkFlag(Flag_Engaged) && !checkFlag(Flag_Ok);
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

    bool checkFlag(const FlagBits p_Flag) const
    {
        return m_Flags & p_Flag;
    }
    void destroy()
    {
        if (!checkFlag(Flag_Engaged))
            return;

        if (checkFlag(Flag_Ok))
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
     * @param p_Args The arguments to pass to the constructor of `E`.
     */
    template <typename... ErrorArgs> static Result Error(ErrorArgs &&...p_Args)
    {
        Result result{};
        result.m_Flags = Flag_Engaged;
        result.m_Error.Construct(std::forward<ErrorArgs>(p_Args)...);
        return result;
    }

    Result(const E &p_Error) : m_Flags(Flag_Engaged)
    {
        m_Error.Construct(p_Error);
    }
    Result(E &&p_Error) : m_Flags(Flag_Engaged)
    {
        m_Error.Construct(std::move(p_Error));
    }

    template <typename Error>
        requires(!std::same_as<E, Error>)
    Result(const Result<void, Error> &p_Other) : m_Flags(p_Other.m_Flags)
    {
        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlag(Flag_Ok), "[TOOLKIT] To copy results with different error types but same value types, "
                                        "copy-from result must be a value");
    }
    template <typename Type>
        requires(!std::same_as<void, Type>)
    Result(const Result<Type, E> &p_Other)
        requires(std::copy_constructible<E>)
        : m_Flags(p_Other.m_Flags)
    {
        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlag(Flag_Ok), "[TOOLKIT] To copy results with different value types but same error types, "
                                         "copy-from result must be an error");
        m_Error.Construct(*p_Other.m_Error.Get());
    }

    Result(const Result &p_Other)
        requires(std::copy_constructible<E>)
        : m_Flags(p_Other.m_Flags)
    {
        if (IsError())
            m_Error.Construct(*p_Other.m_Error.Get());
    }
    Result(Result &&p_Other)
        requires(std::move_constructible<E>)
        : m_Flags(p_Other.m_Flags)
    {
        if (IsError())
            m_Error.Construct(std::move(*p_Other.m_Error.Get()));
    }

    Result &operator=(const E &p_Error)
    {
        destroy();
        m_Flags = Flag_Engaged;
        m_Error.Construct(p_Error);
        return *this;
    }
    Result &operator=(E &&p_Error)
    {
        destroy();
        m_Flags = Flag_Engaged;
        m_Error.Construct(p_Error);
        return *this;
    }

    template <typename Error>
        requires(!std::same_as<E, Error>)
    Result &operator=(const Result<void, Error> &p_Other)
    {
        destroy();
        m_Flags = p_Other.m_Flags;
        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlag(Flag_Ok), "[TOOLKIT] To copy results with different error types but same value types, "
                                        "copy-from result must be a value");
        return *this;
    }
    template <typename Type>
        requires(!std::same_as<void, Type>)
    Result &operator=(const Result<Type, E> &p_Other)
        requires(std::copy_constructible<E>)

    {
        destroy();
        m_Flags = p_Other.m_Flags;
        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlag(Flag_Ok), "[TOOLKIT] To copy results with different value types but same error types, "
                                         "copy-from result must be an error");
        m_Error.Construct(*p_Other.m_Error.Get());
        return *this;
    }

    Result &operator=(const Result &p_Other)
        requires(std::is_copy_assignable_v<E>)
    {
        if (this == &p_Other)
            return *this;
        destroy();
        m_Flags = p_Other.m_Flags;
        if (IsError())
            m_Error.Construct(*p_Other.m_Error.Get());

        return *this;
    }
    Result &operator=(Result &&p_Other)
        requires(std::is_move_assignable_v<E>)
    {
        if (this == &p_Other)
            return *this;
        destroy();
        m_Flags = p_Other.m_Flags;
        if (IsError())
            m_Error.Construct(std::move(*p_Other.m_Error.Get()));

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
        return checkFlag(Flag_Engaged) && checkFlag(Flag_Ok);
    }

    bool IsError() const
    {
        return checkFlag(Flag_Engaged) && !checkFlag(Flag_Ok);
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

    bool checkFlag(const FlagBits p_Flag) const
    {
        return m_Flags & p_Flag;
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
    template <typename... ValueArgs> static Result Some(ValueArgs &&...p_Args)
    {
        Result result{};
        result.m_Flags = Flag_Engaged | Flag_Some;
        result.m_Value.Construct(std::forward<ValueArgs>(p_Args)...);
    }
    static Result None()
    {
        Result result{};
        result.m_Flags = Flag_Engaged;
        return result;
    }

    Result(const T &p_Value) : m_Flags(Flag_Engaged | Flag_Some)
    {
        m_Value.Construct(p_Value);
    }
    Result(T &&p_Value) : m_Flags(Flag_Engaged | Flag_Some)
    {
        m_Value.Construct(std::move(p_Value));
    }

    template <typename Error>
        requires(!std::same_as<void, Error>)
    Result(const Result<T, Error> &p_Other)
        requires(std::copy_constructible<T>)
        : m_Flags(p_Other.m_Flags)
    {
        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlag(Flag_Some), "[TOOLKIT] To copy results with different error types but same value types, "
                                          "copy-from result must be a value");

        m_Value.Construct(*p_Other.m_Value.Get());
    }
    template <typename Type>
        requires(!std::same_as<T, Type>)
    Result(const Result<Type, void> &p_Other) : m_Flags(p_Other.m_Flags)
    {
        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlag(Flag_Some), "[TOOLKIT] To copy results with different value types but same error types, "
                                           "copy-from result must be an error");
    }

    Result(const Result &p_Other)
        requires(std::copy_constructible<T>)
        : m_Flags(p_Other.m_Flags)
    {
        if (IsSome())
            m_Value.Construct(*p_Other.m_Value.Get());
    }
    Result(Result &&p_Other)
        requires(std::move_constructible<T>)
        : m_Flags(p_Other.m_Flags)
    {
        if (IsSome())
            m_Value.Construct(std::move(*p_Other.m_Value.Get()));
    }

    Result &operator=(const T &p_Value)
    {
        destroy();
        m_Flags = Flag_Engaged;
        m_Value.Construct(p_Value);
        return *this;
    }
    Result &operator=(T &&p_Value)
    {
        destroy();
        m_Flags = Flag_Engaged;
        m_Value.Construct(p_Value);
        return *this;
    }

    template <typename Error>
        requires(!std::same_as<void, Error>)
    Result &operator=(const Result<T, Error> &p_Other)
        requires(std::copy_constructible<T>)
    {
        destroy();
        m_Flags = p_Other.m_Flags;
        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(checkFlag(Flag_Some), "[TOOLKIT] To copy results with different error types but same value types, "
                                          "copy-from result must be a value");
        m_Value.Construct(*p_Other.m_Value.Get());
        return *this;
    }
    template <typename Type>
        requires(!std::same_as<T, Type>)
    Result &operator=(const Result<Type, void> &p_Other)

    {
        destroy();
        m_Flags = p_Other.m_Flags;
        TKIT_ASSERT(checkFlag(Flag_Engaged), "[TOOLKIT] Cannot copy from an undefined result");
        TKIT_ASSERT(!checkFlag(Flag_Some), "[TOOLKIT] To copy results with different value types but same error types, "
                                           "copy-from result must be an error");
        return *this;
    }

    Result &operator=(const Result &p_Other)
        requires(std::is_copy_assignable_v<T>)
    {
        if (this == &p_Other)
            return *this;
        destroy();
        m_Flags = p_Other.m_Flags;
        if (IsSome())
            m_Value.Construct(*p_Other.m_Value.Get());

        return *this;
    }
    Result &operator=(Result &&p_Other)
        requires(std::is_move_assignable_v<T>)
    {
        if (this == &p_Other)
            return *this;
        destroy();
        m_Flags = p_Other.m_Flags;
        if (IsSome())
            m_Value.Construct(std::move(*p_Other.m_Value.Get()));

        return *this;
    }

    ~Result()
    {
        destroy();
    }

    bool IsSome() const
    {
        return checkFlag(Flag_Engaged) && checkFlag(Flag_Some);
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

    bool checkFlag(const FlagBits p_Flag) const
    {
        return m_Flags & p_Flag;
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
