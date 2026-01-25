#include "tkit/core/pch.hpp"
#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/utils/debug.hpp"
#include <filesystem>
#include <fstream>

namespace TKit::Yaml
{
Node FromString(const std::string_view string)
{
    return YAML::Load(string.data());
}
Node FromFile(const std::string_view path)
{
    TKIT_ASSERT(std::filesystem::exists(path.data()), "File does not exist: {}", path);
    return YAML::LoadFile(path.data());
}
void ToFile(const std::string_view path, const Node &node)
{
    std::ofstream file(path.data());
    file << node;
}
} // namespace TKit::Yaml
