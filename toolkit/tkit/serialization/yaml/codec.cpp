#include "tkit/core/pch.hpp"
#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/utils/logging.hpp"
#include <filesystem>

namespace TKit::Yaml
{
Node LoadFromString(const std::string_view p_String) noexcept
{
    return YAML::Load(p_String.data());
}
Node LoadFromFile(const std::string_view p_Path) noexcept
{
    TKIT_ASSERT(std::filesystem::exists(p_Path.data()), "File does not exist: {}", p_Path);
    return YAML::LoadFile(p_Path.data());
}
} // namespace TKit::Yaml
