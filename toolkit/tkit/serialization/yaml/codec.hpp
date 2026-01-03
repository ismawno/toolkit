#pragma once

#ifndef TKIT_ENABLE_YAML_SERIALIZATION
#    error                                                                                                             \
        "[TOOLKIT][YAML] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_YAML_SERIALIZATION"
#endif

#include "tkit/reflection/reflect.hpp"
#include "tkit/preprocessor/system.hpp"
#include "tkit/utils/alias.hpp"

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_GCC_WARNING_IGNORE("-Wunused-parameter")
TKIT_CLANG_WARNING_IGNORE("-Wunused-parameter")
TKIT_MSVC_WARNING_IGNORE(4100)
#include <yaml-cpp/yaml.h>
TKIT_COMPILER_WARNING_IGNORE_POP()

namespace TKit::Yaml
{
using Node = YAML::Node;

/**
 * @brief This struct encapsulated serialization and deserialization code for a type `T`.
 *
 * To enable serialization for a custom type, a valid specialization of this struct for it must exist. There are many
 * ways to generate it:
 *
 * - *Manual approach*: The simplest. Create a specialization of `Codec` and (de)serialize your type according to your
 * specific needs.

 * - *Automatic generation through reflection API*: If the class has been marked for reflection code generation, such
 * code is visible, and the macro `TKIT_SERIALIZATION_FROM_REFLECTION` has been defined, `Codec` will try to
 * automatically generate (de)serialization code for `T`. This generation will be limited and not very customizable.
 *
 * - *Automatic generation through serialization API*: The most customizable and recommended approach. It will directly
 generate `Codec` specialization for your types, with the possibility of customizing how fields serialize or
 deserialize. Further documentatuion can be found in the `serialize.hpp` file.
 *
 */
template <typename T> struct Codec
{
    static Node Encode(const T &p_Instance)
    {
        Node node;
#ifdef TKIT_SERIALIZATION_FROM_REFLECTION
        static_assert(Reflect<T>::Implemented || std::is_enum_v<T>,
                      "If type has not a dedicated 'Codec<T>' specialization, it must be reflected "
                      "to auto-serialize. It is recommended to use the serialization marks and scripts available.");
        if constexpr (Reflect<T>::Implemented)
            Reflect<T>::ForEachField([&](const auto &p_Field) { node[p_Field.Name] = p_Field.Get(p_Instance); });
        else
        {
            using Integer = std::underlying_type_t<T>;
            if constexpr (std::is_same_v<Integer, u8>)
                node = Node{static_cast<u16>(p_Instance)};
            else
                node = Node{static_cast<Integer>(p_Instance)};
        }
#else
        if constexpr (Reflect<T>::Implemented)
            static_assert(
                std::is_enum_v<T>,
                "By default, the general implementation of Codec<T> only adds automatic (de)serialization of "
                "enums, even if reflection code for T has been generated and is visible when generating this "
                "template (which IS the case). To enable automatic serialization using such reflection, "
                "simply define TKIT_SERIALIZATION_FROM_REFLECTION before including this file. Serialization code "
                "generated this way will be somewhat limited and is not recommended. Use the the serialization marks "
                "and scripts instead, which behave very similarly to the reflection API.");
        else
            static_assert(
                std::is_enum_v<T>,
                "By default, the general implementation of Codec<T> only adds automatic (de)serialization of "
                "enums, even if reflection code for T has been generated and is visible when generating this "
                "template (which is NOT the case). To enable automatic serialization, use the the serialization marks "
                "and scripts available, which is the recommended approach.");
        using Integer = std::underlying_type_t<T>;
        if constexpr (std::is_same_v<Integer, u8>)
            node = Node{static_cast<u16>(p_Instance)};
        else
            node = Node{static_cast<Integer>(p_Instance)};
#endif
        return node;
    }

    static bool Decode(const Node &p_Node, T &p_Instance)
    {
#ifdef TKIT_SERIALIZATION_FROM_REFLECTION
        static_assert(Reflect<T>::Implemented || std::is_enum_v<T>,
                      "If type has not a dedicated 'Codec<T>' specialization, it must be reflected "
                      "to auto-deserialize. It is recommended to use the serialization marks and scripts available.");

        if constexpr (Reflect<T>::Implemented)
            Reflect<T>::ForEachField([&](const auto &p_Field) {
                using Type = TKIT_REFLECT_FIELD_TYPE(p_Field);
                p_Field.Set(p_Instance, p_Node[p_Field.Name].template as<Type>());
            });
        else
        {
            using Integer = std::underlying_type_t<T>;
            p_Instance = static_cast<T>(p_Node.as<Integer>());
        }
#else
        if constexpr (Reflect<T>::Implemented)
            static_assert(
                std::is_enum_v<T>,
                "By default, the general implementation of Codec<T> only adds automatic (de)serialization of "
                "enums, even if reflection code for T has been generated and is visible when generating this "
                "template (which IS the case). To enable automatic serialization using such reflection, "
                "simply define TKIT_SERIALIZATION_FROM_REFLECTION before including this file. Serialization code "
                "generated this way will be somewhat limited and is not recommended. Use the the serialization marks "
                "and scripts instead, which behave very similarly to the reflection API.");
        else
            static_assert(
                std::is_enum_v<T>,
                "By default, the general implementation of Codec<T> only adds automatic (de)serialization of "
                "enums, even if reflection code for T has been generated and is visible when generating this "
                "template (which is NOT the case). To enable automatic serialization, use the the serialization marks "
                "and scripts available, which is the recommended approach.");

        using Integer = std::underlying_type_t<T>;
        p_Instance = static_cast<T>(p_Node.as<Integer>());
#endif
        return true;
    }
};

Node FromString(std::string_view p_String);
Node FromFile(std::string_view p_Path);

void ToFile(std::string_view p_Path, const Node &p_Node);

template <typename T> void Serialize(const std::string_view p_Path, const T &p_Instance)
{
    const Node node{p_Instance};
    ToFile(p_Path, node);
}
template <typename T> T Deserialize(const std::string_view p_Path)
{
    const Node node = FromFile(p_Path);
    return node.as<T>();
}

} // namespace TKit::Yaml

namespace YAML
{
template <typename T> struct convert
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
} // namespace YAML
