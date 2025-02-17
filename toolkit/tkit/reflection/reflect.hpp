#pragma once

#define TKIT_REFLECT(p_ClassName) friend class TKit::Reflect<p_ClassName>;

namespace TKit
{
template <typename T> class Reflect
{
    static constexpr bool Implemented = false;
};
}; // namespace TKit