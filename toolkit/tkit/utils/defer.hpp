#pragma once

#include "tkit/utils/non_copyable.hpp"

#define TKIT_DEFER_CAPTURE(p_Deletor, ...) const TKit::Defer __tkit_defer##__LINE__##{[__VA_ARGS__] { p_Deletor; }};
#define TKIT_DEFER(p_Deletor) const TKit::Defer __tkit_defer##__LINE__##{[&] { p_Deletor; }};

namespace TKit
{
template <typename F> struct Defer
{
    TKIT_NON_COPYABLE(Defer)
    Defer(F &&p_Func) : Func(std::move(p_Func))
    {
    }

    ~Defer()
    {
        Func();
    }

    F Func;
};
} // namespace TKit
