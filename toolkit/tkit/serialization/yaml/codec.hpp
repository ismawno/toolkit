#pragma once

#ifndef TKIT_ENABLE_YAML_SERIALIZATION
#    error                                                                                                             \
        "[TOOLKIT][YAML] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_YAML_SERIALIZATION"
#endif

#include "tkit/reflection/reflect.hpp"
#include "tkit/serialization/yaml/tree.hpp"

namespace TKit
{
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
    static void Encode(YamlNode &node, const T &instance)
    {
#ifdef TKIT_SERIALIZATION_FROM_REFLECTION
        static_assert(Reflect<T>::Implemented || std::is_enum_v<T>,
                      "If type has not a dedicated 'Codec<T>' specialization, it must be reflected "
                      "to auto-serialize. It is recommended to use the serialization marks and scripts available.");
        if constexpr (Reflect<T>::Implemented)
            Reflect<T>::ForEachField([&](const auto &field) { node[field.Name] << field.Get(instance); });
        else
        {
            using Integer = std::underlying_type_t<T>;
            if constexpr (std::is_same_v<Integer, u8>)
                node << u16(instance);
            else
                node << Integer(instance);
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
            node << u16(instance);
        else
            node << Integer(instance);
#endif
    }

    static YamlReadResult Decode(const YamlNode &node, T &instance)
    {
#ifdef TKIT_SERIALIZATION_FROM_REFLECTION
        static_assert(Reflect<T>::Implemented || std::is_enum_v<T>,
                      "If type has not a dedicated 'Codec<T>' specialization, it must be reflected "
                      "to auto-deserialize. It is recommended to use the serialization marks and scripts available.");

        if constexpr (Reflect<T>::Implemented)
        {
            YamlReadResult res = YamlReadResult::Ok();
            Reflect<T>::ForEachField([&](const auto &field) {
                using Type = TKIT_REFLECT_FIELD_TYPE(field);
                if (!res)
                    return;
                Type val;
                res = node[field.Name].TryRead(val);
                if (res)
                    field.Set(instance, val);
            });
        }
        else
        {
            using Integer = std::underlying_type_t<T>;
            Integer i;
            const YamlReadResult res = node.Read(i);
            if (!res)
                return res;
            instance = T(i);
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
        Integer i;
        const YamlReadResult res = node.Read(i);
        if (!res)
            return res;
        instance = T(i);
#endif
    }
};

template <typename T> void Serialize(const fs::path &path, const T &instance)
{
    const YamlTree tree{};
    tree.GetRoot() << instance;
    tree.ToFile(path);
}
template <typename T> YamlReadResult TryDeserialize(const fs::path &path, T &instance)
{
    const YamlTree tree = YamlTree::FromFile(path);
    return tree.GetRoot().TryRead(instance);
}
template <typename T> void Deserialize(const fs::path &path, T &instance)
{
    const YamlTree tree = YamlTree::FromFile(path);
    tree.GetRoot().Read(instance);
}
template <typename T, typename... Args> T Deserialize(const fs::path &path, Args &&...args)
{
    const YamlTree tree = YamlTree::FromFile(path);
    return tree.GetRoot().Read<T>(std::forward<Args>(args)...);
}

template <typename T>
concept BuiltInCodecable =
    Numeric<T> || std::is_same_v<std::remove_cvref_t<T>, char *> || std::is_same_v<std::remove_cvref_t<T>, std::string>;

template <BuiltInCodecable T> struct Codec<T>
{
    static void Encode(YamlNode &node, const T &instance);
    static YamlReadResult Decode(const YamlNode &node, T &instance);
};

} // namespace TKit
