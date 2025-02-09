#include "tkit/utils/alias.hpp"

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_GCC_WARNING_IGNORE("-Wunused-parameter")
TKIT_CLANG_WARNING_IGNORE("-Wunused-parameter")
TKIT_MSVC_WARNING_IGNORE(4100)
#include <yaml-cpp/yaml.h>
TKIT_COMPILER_WARNING_IGNORE_POP()

namespace TKit::Yaml
{
template <typename T> struct Codec;
using Node = YAML::Node;

Node LoadFromString(std::string_view p_String);
Node LoadFromFile(std::string_view p_Path);

template <typename T> T Parse(const Node &p_Node)
{
    return p_Node.as<T>();
}
template <typename T, std::convertible_to<T> D> T Parse(const Node &p_Node, D &&p_Default)
{
    return p_Node.as<T>(std::forward<D>(p_Default));
}

template <typename T, std::integral I = u32> T ParseEnum(const Node &p_Node)
{
    return static_cast<T>(p_Node.as<I>());
}
template <typename T, std::convertible_to<T> D, std::integral I = u32> T ParseEnum(const Node &p_Node, D &&p_Default)
{
    return static_cast<T>(p_Node.as<I>(std::forward<D>(p_Default)));
}

} // namespace TKit::Yaml

template <typename T> struct YAML::convert
{
    static Node encode(const T &rhs)
    {
        return TKit::Yaml::Codec<T>::Encode(rhs);
    }

    static bool decode(const Node &node, T &rhs)
    {
        return TKit::Yaml::Codec<T>::Decode(node, rhs);
    }
};
