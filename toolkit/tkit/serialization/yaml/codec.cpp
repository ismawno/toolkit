#include "tkit/core/pch.hpp"
#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/utils/debug.hpp"
#include <filesystem>
#include <fstream>

namespace TKit::Yaml
{
Node FromString(const char *string)
{
    return YAML::Load(string);
}
Node FromFile(const char *path)
{
    TKIT_ASSERT(std::filesystem::exists(std::string(path)), "File does not exist: {}", path);
    return YAML::LoadFile(path);
}
void ToFile(const char *path, const Node &node)
{
    std::ofstream file(path);
    file << node;
}
Node FromString(const std::string &string)
{
    return YAML::Load(string);
}
Node FromFile(const std::string &path)
{
    TKIT_ASSERT(std::filesystem::exists(std::string(path)), "File does not exist: {}", path);
    return YAML::LoadFile(path);
}
void ToFile(const std::string &path, const Node &node)
{
    std::ofstream file(path);
    file << node;
}
} // namespace TKit::Yaml
