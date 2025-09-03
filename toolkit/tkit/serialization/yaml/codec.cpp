#include "tkit/core/pch.hpp"
#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/utils/logging.hpp"
#include <filesystem>

namespace TKit::Yaml
{
Node FromString(const std::string_view p_String)
{
    return YAML::Load(p_String.data());
}
Node FromFile(const std::string_view p_Path)
{
    TKIT_ASSERT(std::filesystem::exists(p_Path.data()), "File does not exist: {}", p_Path);
    return YAML::LoadFile(p_Path.data());
}
void ToFile(const std::string_view p_Path, const Node &p_Node)
{
    std::ofstream file(p_Path.data());
    file << p_Node;
}
} // namespace TKit::Yaml
