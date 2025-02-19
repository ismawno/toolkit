#include "tkit/core/pch.hpp"
#include "tkit/serialization/yaml/codec.hpp"

namespace TKit::Yaml
{
Node LoadFromString(const std::string_view p_String) noexcept
{
    return YAML::Load(p_String.data());
}
Node LoadFromFile(const std::string_view p_Path) noexcept
{
    return YAML::LoadFile(p_Path.data());
}
} // namespace TKit::Yaml