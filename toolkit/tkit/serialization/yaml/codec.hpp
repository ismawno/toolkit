#pragma once

#ifndef TKIT_ENABLE_YAML_SERIALIZATION
#    error "To include this file, the corresponding feature must be enabled with TOOLKIT_ENABLE_YAML_SERIALIZATION"
#endif

#include "tkit/utils/alias.hpp"
#include "tkit/reflection/reflect.hpp"
#include "tkit/utils/logging.hpp"
#include "tkit/container/span.hpp"
#include "tkit/utils/concepts.hpp"

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_GCC_WARNING_IGNORE("-Wunused-parameter")
TKIT_CLANG_WARNING_IGNORE("-Wunused-parameter")
TKIT_MSVC_WARNING_IGNORE(4100)
#include <yaml-cpp/yaml.h>
TKIT_COMPILER_WARNING_IGNORE_POP()

namespace TKit::Yaml
{
using Node = YAML::Node;
template <typename T> struct Codec
{
    static Node Encode(const T &p_Instance) noexcept
    {
        static_assert(Reflect<T>::Implemented || std::is_enum_v<T>,
                      "If type has not a dedicated 'Codec<T>' specialization, it must be reflected "
                      "to auto-serialize");
        Node node;
        if constexpr (Reflect<T>::Implemented)
            Reflect<T>::ForEachField(
                [&node, &p_Instance](const auto &p_Field) { node[p_Field.Name] = p_Field.Get(p_Instance); });
        else
        {
            using Integer = std::underlying_type_t<T>;
            node = Node{static_cast<Integer>(p_Instance)};
        }
        return node;
    }

    static bool Decode(const Node &p_Node, T &p_Instance) noexcept
    {
        static_assert(
            Reflect<T>::Implemented || std::is_enum_v<T>,
            "If type has not a dedicated 'Codec<T>' specialization, it must be reflected to auto-deserialize");
        if constexpr (Reflect<T>::Implemented)
            Reflect<T>::ForEachField([&p_Node, &p_Instance](const auto &p_Field) {
                using Field = decltype(p_Field);
                using Type = typename NoCVRef<Field>::Type;
                p_Field.Set(p_Instance, p_Node.as<Type>());
            });
        else
        {
            using Integer = std::underlying_type_t<T>;
            p_Instance = static_cast<T>(p_Node.as<Integer>());
        }
        return true;
    }
};

template <typename T, typename FContainer>
void SerializeFields(Node &p_Node, const T &p_Instance, const FContainer &p_Fields) noexcept
{
    static_assert(Reflect<T>::Implemented, "Type must be reflected to serialize fields");
    Reflect<T>::ForEachField(
        p_Fields, [&p_Node, &p_Instance](const auto &p_Field) { p_Node[p_Field.Name] = p_Field.Get(p_Instance); });
}
template <typename T, typename FContainer>
void DeserializeFields(const Node &p_Node, T &p_Instance, const FContainer &p_Fields) noexcept
{
    static_assert(Reflect<T>::Implemented, "Type must be reflected to deserialize fields");
    Reflect<T>::ForEachField(p_Fields, [&p_Node, &p_Instance](const auto &p_Field) {
        using Field = decltype(p_Field);
        using Type = typename NoCVRef<Field>::Type;
        p_Field.Set(p_Instance, p_Node[p_Field.Name].template as<Type>());
    });
}

Node LoadFromString(std::string_view p_String) noexcept;
Node LoadFromFile(std::string_view p_Path) noexcept;

} // namespace TKit::Yaml

template <typename T> struct YAML::convert
{
    static Node encode(const T &p_Instance)
    {
        return TKit::Yaml::Codec<T>::Encode(p_Instance);
    }

    static bool decode(const Node &p_Node, T &p_Instance)
    {
        return TKit::Yaml::Codec<T>::Decode(p_Node, p_Instance);
    }
};
