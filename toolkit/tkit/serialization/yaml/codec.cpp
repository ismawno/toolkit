#include "tkit/core/pch.hpp"
#include "tkit/serialization/yaml.hpp"

namespace TKit::Yaml
{
Node LoadFromString(std::string_view p_String)
{
    return YAML::Load(p_String.data());
}
Node LoadFromFile(std::string_view p_Path)
{
    return YAML::LoadFile(p_Path.data());
}
} // namespace TKit::Yaml