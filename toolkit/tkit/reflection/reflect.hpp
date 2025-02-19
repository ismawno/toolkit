#pragma once

#define TKIT_REFLECT_DECLARE(p_ClassName) friend class TKit::Reflect<p_ClassName>;

#define TKIT_REFLECT_IGNORE_BEGIN
#define TKIT_REFLECT_IGNORE_END

#define TKIT_REFLECT_GROUP_BEGIN(p_GroupName)
#define TKIT_REFLECT_GROUP_END()

namespace TKit
{
template <typename T> class Reflect
{
    static constexpr bool Implemented = false;
};
}; // namespace TKit