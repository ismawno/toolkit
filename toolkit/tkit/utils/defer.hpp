#pragma once

#include "tkit/utils/non_copyable.hpp"
#include "tkit/preprocessor/utils.hpp"

#define TKIT_DEFER_CAPTURE(deletor, ...) const TKit::Defer __tkit_defer##__LINE__{[__VA_ARGS__] { deletor; }};
#define TKIT_DEFER(deletor) const TKit::Defer __tkit_defer##__LINE__{[&] { deletor; }};

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
} // namespace TKit
