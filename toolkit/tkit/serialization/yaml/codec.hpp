#pragma once

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
 * - *Manual approach*: The simplest. Create a specialization of `YamlCodec` and (de)serialize your type according to
 your
 * specific needs.

 * - *Automatic generation through reflection API*: If the class has been marked for reflection code generation, such
 * code is visible, and the macro `TKIT_SERIALIZATION_FROM_REFLECTION` has been defined, `YamlCodec` will try to
 * automatically generate (de)serialization code for `T`. This generation will be limited and not very customizable.
 *
 * - *Automatic generation through serialization API*: The most customizable and recommended approach. It will directly
 generate `YamlCodec` specialization for your types, with the possibility of customizing how fields serialize or
 deserialize. Further documentatuion can be found in the `serialize.hpp` file.
 *
 */
template <typename T> struct YamlCodec
{
    static void Encode(YamlNode &node, const T &instance)
    {
#ifdef TKIT_SERIALIZATION_FROM_REFLECTION
        static_assert(Reflect<T>::Implemented || std::is_enum_v<T>,
                      "If type has not a dedicated 'YamlCodec<T>' specialization, it must be reflected "
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
                "By default, the general implementation of YamlCodec<T> only adds automatic (de)serialization of "
                "enums, even if reflection code for T has been generated and is visible when generating this "
                "template (which IS the case). To enable automatic serialization using such reflection, "
                "simply define TKIT_SERIALIZATION_FROM_REFLECTION before including this file. Serialization code "
                "generated this way will be somewhat limited and is not recommended. Use the the serialization marks "
                "and scripts instead, which behave very similarly to the reflection API.");
        else
            static_assert(
                std::is_enum_v<T>,
                "By default, the general implementation of YamlCodec<T> only adds automatic (de)serialization of "
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

    TKIT_NO_DISCARD static YamlReadResult Decode(const ConstYamlNode &node, T &instance)
    {
#ifdef TKIT_SERIALIZATION_FROM_REFLECTION
        static_assert(Reflect<T>::Implemented || std::is_enum_v<T>,
                      "If type has not a dedicated 'YamlCodec<T>' specialization, it must be reflected "
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
            return res;
        }
        else
        {
            using Integer = std::underlying_type_t<T>;
            Integer i;
            const YamlReadResult res = node.TryRead(i);
            if (res)
                instance = T(i);
            return res;
        }
#else
        if constexpr (Reflect<T>::Implemented)
            static_assert(
                std::is_enum_v<T>,
                "By default, the general implementation of YamlCodec<T> only adds automatic (de)serialization of "
                "enums, even if reflection code for T has been generated and is visible when generating this "
                "template (which IS the case). To enable automatic serialization using such reflection, "
                "simply define TKIT_SERIALIZATION_FROM_REFLECTION before including this file. Serialization code "
                "generated this way will be somewhat limited and is not recommended. Use the the serialization marks "
                "and scripts instead, which behave very similarly to the reflection API.");
        else
            static_assert(
                std::is_enum_v<T>,
                "By default, the general implementation of YamlCodec<T> only adds automatic (de)serialization of "
                "enums, even if reflection code for T has been generated and is visible when generating this "
                "template (which is NOT the case). To enable automatic serialization, use the the serialization marks "
                "and scripts available, which is the recommended approach.");

        using Integer = std::underlying_type_t<T>;
        Integer i;
        const YamlReadResult res = node.TryRead(i);
        if (res)
            instance = T(i);
        return res;
#endif
    }
};

template <typename T> TKIT_NO_DISCARD FileSystemResult<> Serialize(const YamlPath &path, const T &instance)
{
    YamlTree tree{};
    tree.GetRoot() << instance;
    return tree.ToFile(path);
}
template <typename T> TKIT_NO_DISCARD YamlReadResult Deserialize(const YamlPath &path, T &instance)
{
    const auto tree = YamlTree::FromFile(path);
    if (!tree)
        return YamlReadResult::Error(NullYamlNodeId, YamlString::Format("File to deserialize at '{}' with error: {}",
                                                                        path, ToString(tree.GetError())));

    return tree->GetRoot().TryRead(instance);
}
template <typename T, typename... Args>
    requires(sizeof...(Args) != 1 || !(std::is_same_v<std::remove_cvref_t<Args>, T> || ... || false))
TKIT_NO_DISCARD Result<T, YamlReadError> Deserialize(const YamlPath &path, Args &&...args)
{
    T instance{std::forward<Args>(args)...};
    const auto res = Deserialize(path, instance);
    TKIT_RETURN_ON_ERROR(res);
    return instance;
}

template <typename T>
concept BuiltInCodecable =
    Numeric<T> || std::is_same_v<std::remove_cvref_t<T>, bool> || std::is_same_v<std::remove_cvref_t<T>, std::string>;

template <BuiltInCodecable T> struct YamlCodec<T>
{
    static void Encode(YamlNode &node, const T &instance)
    {
        Detail::EncodeBuiltIn(node, instance);
    }
    TKIT_NO_DISCARD static YamlReadResult Decode(const ConstYamlNode &node, T &instance)
    {
        return Detail::DecodeBuiltIn(node, instance);
    }
};

template <> struct YamlCodec<const char *>
{
    static void Encode(YamlNode &node, const char *str);
};
template <> struct YamlCodec<char *>
{
    static void Encode(YamlNode &node, const char *str)
    {
        YamlCodec<const char *>::Encode(node, str);
    }
};

template <usize N> struct YamlCodec<char[N]>
{
    static void Encode(YamlNode &node, const char *str)
    {
        return YamlCodec<const char *>::Encode(node, str);
    }
};

template <> struct YamlCodec<std::string_view>
{
    static void Encode(YamlNode &node, const std::string_view &instance);
};

} // namespace TKit
