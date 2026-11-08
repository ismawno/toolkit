#pragma once

#include "tkit/utils/non_copyable.hpp"
#include "tkit/preprocessor/utils.hpp"

#define TKIT_DEFER(deletor)                                                                                            \
    const TKit::Defer TKIT_CONCAT(__tkit_defer, __COUNTER__)                                                           \
    {                                                                                                                  \
        [&] { deletor; }                                                                                               \
    }

#define TKIT_ERROR_DEFER_SCOPE()                                                                                       \
    TKit::ErrorDeferScope __tkit_error_defer_scope                                                                     \
    {                                                                                                                  \
    }

#define TKIT_ERROR_DEFER(...)                                                                                          \
    const TKit::ErrorDefer TKIT_CONCAT(__tkit_error_defer, __COUNTER__)                                                \
    {                                                                                                                  \
        __tkit_error_defer_scope, [&] { __VA_ARGS__; }                                                                 \
    }

#define TKIT_ERROR_DEFER_COMMIT() __tkit_error_defer_scope.Commit()

// for `return SomeCall(...);` where the returned Result itself may be an error
#define TKIT_ERROR_DEFER_RETURN(expression)                                                                            \
    const auto __tkit_error_defer_result = (expression);                                                               \
    if (__tkit_error_defer_result)                                                                                     \
        __tkit_error_defer_scope.Commit();                                                                             \
    return __tkit_error_defer_result

namespace TKit
{
template <typename F> struct Defer
{
    TKIT_NON_COPYABLE(Defer)
    Defer(F &&func) : Func(std::move(func))
    {
    }

    ~Defer()
    {
        Func();
    }

    F Func;
};
class ErrorDeferScope
{
  public:
    TKIT_NON_COPYABLE(ErrorDeferScope)
    ErrorDeferScope() = default;

    void Commit()
    {
        m_Committed = true;
    }
    bool IsCommitted() const
    {
        return m_Committed;
    }

  private:
    bool m_Committed = false;
};

template <typename F> struct ErrorDefer
{
    TKIT_NON_COPYABLE(ErrorDefer)
    ErrorDefer(const ErrorDeferScope &scope, F &&func) : Scope(scope), Func(std::move(func))
    {
    }
    ~ErrorDefer()
    {
        if (!Scope.IsCommitted())
            Func();
    }

    const ErrorDeferScope &Scope;
    F Func;
};
} // namespace TKit
