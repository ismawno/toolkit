from generator import CPPGenerator
from parser import ControlMacros, MacroPair, Field, FieldCollection, ClassCollection
from orchestrator import CPPOrchestrator
from argparse import ArgumentParser, Namespace
from pathlib import Path
from collections.abc import Callable

import sys

sys.path.append(str(Path(__file__).parent.parent.parent))

from convoy import Convoy


def parse_arguments() -> Namespace:
    desc = """
    This python script takes in a C++ file and scans it for classes/structs marked with the
    toolkit macro TKIT_REFLECT_DECLARE. If it finds any instance of this macro, it will generate
    another C++ file containing a template specialization of a special reflect class (called Reflect)
    which will contain compile and run time reflection information about the class/struct's fields.

    It is also possible to group fields with the macros TKIT_REFLECT_GROUP_BEGIN and TKIT_REFLECT_GROUP_END,
    and special functions to only retrieve fields under this group will be created.

    If some fields must be left out, the macros TKIT_REFLECT_IGNORE_BEGIN and TKIT_REFLECT_IGNORE_END can also
    be used.
    """
    parser = ArgumentParser(description=desc)
    CPPOrchestrator.setup_cli_arguments(parser, add_verbose=True)
    return parser.parse_args()


Convoy.program_label = "REFLECT"
args = parse_arguments()
Convoy.is_verbose = args.verbose

gpair = MacroPair(
    "TKIT_REFLECT_GROUP_BEGIN",
    "TKIT_REFLECT_GROUP_END",
)
ipair = MacroPair(
    "TKIT_REFLECT_IGNORE_BEGIN",
    "TKIT_REFLECT_IGNORE_END",
)
macros = ControlMacros(
    "TKIT_REFLECT_DECLARE",
    "TKIT_REFLECT_DECLARE_ENUM",
    gpair,
    ipair,
)

orchestrator = CPPOrchestrator.from_cli_arguments(args, macros=macros, reserved_group_names="Static")


def generate_reflection_code(hpp: CPPGenerator, classes: ClassCollection, /) -> None:
    hpp.include("tkit/container/array.hpp", quotes=True)
    hpp.include("tkit/reflection/reflect.hpp", quotes=True)
    hpp.include("tkit/utils/concepts.hpp", quotes=True)
    hpp.include("tkit/utils/logging.hpp", quotes=True)
    hpp.include("tuple")
    hpp.include("string_view")

    with hpp.scope("namespace TKit", indent=0):
        for enum in classes.enums:
            for namespace in enum.namespaces:
                if namespace != "TKit":
                    hpp(f"using namespace {namespace};", unique_line=True)

            with hpp.doc():
                hpp.brief(
                    f"This is an auto-generated specialization of the placeholder `Reflect` class containing information about the enum `{enum.id.identifier}`."
                )
                hpp("It includes basic implementations to transform an enum to a string or viceversa.")
            with hpp.scope(
                f"template <> class Reflect<{enum.id.identifier}>",
                closer="};",
            ):
                hpp("public:")
                hpp("static constexpr bool Implemented = true;")

                with hpp.doc():
                    hpp.brief("Get an enum value from a string.")
                    hpp("If no valid enum value is found, the first enum value will be returned. Take care.")

                with hpp.scope(f"constexpr {enum.id.identifier} FromString(const std::string_view p_Value) noexcept"):
                    vals = list(enum.values.keys())
                    for val in vals:
                        with hpp.scope(f'if (p_Value == "{val}")', delimiters=False):
                            hpp(f"return {enum.id.identifier}::{val};")

                    hpp('TKIT_ERROR("Found no valid enum value for the string {}.", p_Value);')
                    hpp(f"return {enum.id.identifier}::{vals[0]};")

                with hpp.doc():
                    hpp.brief("Transform an enum value to a string.")
                    hpp("If the enum is not valid, a null pointer will be returned.")

                with hpp.scope(f"constexpr const char *ToString(const {enum.id.identifier} p_Value) noexcept"):
                    with hpp.scope("switch(p_Value)"):
                        for val in enum.values:
                            with hpp.scope(f"case {enum.id.identifier}::{val}:", delimiters=False):
                                hpp(f'return "{val}";')

                        with hpp.scope("default:", delimiters=False):
                            hpp("return nullptr;")

        for clsinfo in classes.classes:
            for namespace in clsinfo.namespaces:
                if namespace != "TKit":
                    hpp(f"using namespace {namespace};", unique_line=True)

            with hpp.doc():
                hpp.brief(
                    f"This is an auto-generated specialization of the placeholder `Reflect` class containing compile-time and run-time information about `{clsinfo.id.identifier}`."
                )
                hpp(
                    "With this specialization, you may query information about the class or struct fields iteratively. If used as a default serialization generation with `TKit::Codec` struct, this file must be included as well before template instantiations of `TKit::Codec` occur."
                )
            with hpp.scope(
                f"template <{clsinfo.id.templdecl if clsinfo.id.templdecl is not None else ''}> class Reflect<{clsinfo.id.identifier}>",
                closer="};",
            ):
                hpp("public:")
                hpp("static constexpr bool Implemented = true;")
                with hpp.scope("enum class FieldVisibility : u8", closer="};"):
                    hpp("Private = 0,")
                    hpp("Protected = 1,")
                    hpp("Public = 2")

                memfields = FieldCollection()
                statfields = FieldCollection()
                for f in clsinfo.fields.filter_modifier(exclude="static"):
                    memfields.add(f)
                for f in clsinfo.fields.filter_modifier(include="static"):
                    statfields.add(f)

                hpp(f"static constexpr bool HasMemberFields = {'true' if memfields.fields else 'false'};")
                hpp(f"static constexpr bool HasStaticFields = {'true' if statfields.fields else 'false'};")

                def generate_reflect_body(fcollection: FieldCollection, /, *, is_static: bool) -> None:
                    modifier = "Static" if is_static else "Member"

                    hpp("public:")
                    dtype = "u8" if len(fcollection.per_group) < 256 else "u16"
                    if fcollection.per_group:
                        with hpp.doc():
                            hpp.brief(
                                f"This enum lists all of the groups defined with the macro pair `{gpair.begin}` and `{gpair.end}`."
                            )
                            hpp("Its values may be used as compile-time filters in certain functions.")

                        with hpp.scope(f"enum class {modifier}Group : {dtype}", closer="};"):
                            for i, group in enumerate(fcollection.per_group):
                                hpp(f"{group} = {i},")

                    with hpp.doc():
                        hpp.brief(
                            f"Encapsulates specific information about a `{clsinfo.id.identifier}` {modifier.lower()} field."
                        )
                        hpp("You may access an arbitrary field's value at run-time with the help of this struct.")
                        hpp(
                            "A field may be null, which means it does not represent any real field. You may check for its validity using the bool operator. This is only needed when querying a specific field by name."
                        )

                    with hpp.scope(f"template <typename Ref_Type> struct {modifier}Field", closer="};"):
                        hpp("using Type = Ref_Type;")
                        hpp("const char *Name = nullptr;")
                        hpp("const char *TypeString = nullptr;")
                        if is_static:
                            hpp("Ref_Type *Pointer = nullptr;")
                        else:
                            hpp(f"Ref_Type {clsinfo.id.identifier}::* Pointer = nullptr;")
                        hpp("FieldVisibility Visibility{};")

                        with hpp.scope("operator bool() const noexcept"):
                            hpp("return Name && TypeString && Pointer;")

                        if not is_static:

                            def create_get_doc() -> None:
                                with hpp.doc():
                                    hpp.brief(
                                        f"Given an instance of `{clsinfo.id.identifier}`, access the value of the field that is currently represented."
                                    )
                                    hpp.param("p_Instance", f"An instance of type `{clsinfo.id.identifier}`.")
                                    hpp.ret(f"The value of the field for the instance.")

                            create_get_doc()
                            with hpp.scope(f"Ref_Type &Get({clsinfo.id.identifier} &p_Instance) const noexcept"):
                                hpp("return p_Instance.*Pointer;")

                            create_get_doc()
                            with hpp.scope(
                                f"const Ref_Type &Get(const {clsinfo.id.identifier} &p_Instance) const noexcept"
                            ):
                                hpp("return p_Instance.*Pointer;")

                            with hpp.doc():
                                hpp.brief(
                                    f"Given an instance of `{clsinfo.id.identifier}`, set the value of the field that is currently represented."
                                )
                                hpp.param("p_Instance", f"An instance of type `{clsinfo.id.identifier}`.")
                                hpp.param("p_Value", "The value to set.")
                            with hpp.scope(
                                f"template <std::convertible_to<Ref_Type> U> void Set({clsinfo.id.identifier} &p_Instance, U &&p_Value) const noexcept"
                            ):
                                hpp("p_Instance.*Pointer = std::forward<U>(p_Value);")

                    def create_if_constexpr_per_type(
                        fn: Callable[..., None],
                        null: str,
                        /,
                        *,
                        group: str | None = None,
                        as_sequence: bool = True,
                        delimiters: bool = False,
                    ) -> None:
                        cnd = "if"
                        for vtype, fields in fcollection.per_type.items():
                            if as_sequence:
                                fields = create_cpp_fields_sequence(fields, group=group)
                            if not fields:
                                continue
                            with hpp.scope(
                                f"{cnd} constexpr (std::is_same_v<Ref_Type, {vtype}>)",
                                delimiters=delimiters,
                            ):
                                fn(fields, vtype)
                            cnd = "else if"
                        if cnd == "else if":
                            with hpp.scope("else", delimiters=False):
                                hpp(f"return {null};")
                        else:
                            hpp(f"return {null};")

                    def convert_field_to_cpp(field: Field, /) -> str:
                        def replacer(vtype: str, /) -> str:
                            return vtype.replace('"', r"\"")

                        return f'{modifier}Field<{field.vtype}>{{"{field.name}", "{replacer(field.vtype)}", &{clsinfo.id.identifier}::{field.name}, FieldVisibility::{field.visibility.capitalize()}}}'

                    def create_cpp_fields_sequence(fields: list[Field], /, *, group: str | None = None) -> list[str]:
                        return [
                            convert_field_to_cpp(field) for field in fields if group is None or group in field.groups
                        ]

                    with hpp.doc():
                        hpp.brief("Get a specific field by name.")
                        hpp("If the field does not exist, an empty field will be returned.")
                        hpp.tparam(
                            "Ref_Type", "The type of the field. A global, type agnostic query is not supported."
                        )
                        hpp.param("p_Field", "The name of the field.")

                    with hpp.scope(
                        f"template <typename Ref_Type> static auto Get{modifier}Field(const std::string_view p_Field) noexcept"
                    ):

                        def generator(fields: list[Field], vtype: str, /) -> None:
                            for field in fields:
                                with hpp.scope(f'if (p_Field == "{field.name}")', delimiters=False):
                                    hpp(f"return {convert_field_to_cpp(field)};")
                            hpp(f"return {modifier}Field<{vtype}>{{}};")

                        create_if_constexpr_per_type(
                            generator, f"{modifier}Field<u8>{{}}", as_sequence=False, delimiters=True
                        )

                    with hpp.scope(f"static bool Has{modifier}Field(const std::string p_Field) noexcept"):
                        for vtype in fcollection.per_type:
                            with hpp.scope(f"if (Get{modifier}Field<{vtype}>(p_Field))", delimiters=False):
                                hpp("return true;")

                        hpp("return false;")

                    def create_tuple_sequence(fields: list[str], /, **_) -> str:
                        return f"std::make_tuple({', '.join(fields)})"

                    def create_array_sequence(fields: list[str], /, *, vtype: str | None = None) -> str:
                        if vtype is None:
                            vtype = "Ref_Type"
                        return f"Array<{modifier}Field<{vtype}>, {len(fields)}>{{{', '.join(fields)}}}"

                    def create_get_fields_method(fields: list[Field], /, *, group: str = "") -> None:
                        fields_cpp = create_cpp_fields_sequence(fields)

                        with hpp.doc():
                            msg = f"Get all {modifier.lower()} fields"
                            if group:
                                msg += f" for the `{group}` group."
                            else:
                                msg += "."
                            hpp.brief(msg)
                            hpp.tparam(
                                "Ref_Types",
                                "You may optionally provide type filters so that only fields with such types are retrieved.",
                            )
                            hpp.ret(
                                f"If the retrieved fields share all the same type, the return value will be simplified to a `TKit::Array<{modifier}Field>`. If that is not the case however, the fields will be stored in a `std::tuple`."
                            )

                        with hpp.scope(
                            f"template <typename... Ref_Types> static constexpr auto Get{modifier}{group}Fields() noexcept"
                        ):
                            with hpp.scope("if constexpr (sizeof...(Ref_Types) == 0)", delimiters=False):
                                hpp(f"return {create_tuple_sequence(fields_cpp)};")
                            with hpp.scope(
                                "else if constexpr (sizeof...(Ref_Types) == 1)",
                                delimiters=False,
                            ):
                                hpp(f"return get{modifier}{group}Array<Ref_Types...>();")
                            with hpp.scope("else", delimiters=False):
                                hpp(f"return std::tuple_cat(get{modifier}{group}Tuple<Ref_Types>()...);")

                    def create_for_each_method(*, group: str = "") -> None:
                        with hpp.doc():
                            msg = f"Iterate over all {modifier.lower()} fields"
                            if group:
                                msg += f" for the `{group}` group."
                            else:
                                msg += "."
                            hpp.brief(msg)
                            hpp.tparam(
                                "Ref_Types",
                                "You may optionally provide type filters so that only fields with such types are retrieved.",
                            )
                            hpp.param(
                                "p_Fun",
                                "A callable object that must accept any field type. The most straightforward way of doing so is by declaring the macro as `[](const auto &p_Field){};`.",
                            )

                        with hpp.scope(
                            f"template <typename... Ref_Types, typename Ref_Fun> static constexpr void ForEach{modifier}{group}Field(Ref_Fun &&p_Fun) noexcept"
                        ):
                            hpp(f"const auto fields = Get{modifier}{group}Fields<Ref_Types...>();")
                            hpp(f"ForEach{modifier}Field(fields, std::forward<Ref_Fun>(p_Fun));")

                    with hpp.doc():
                        hpp.brief(f"Iterate over all {modifier.lower()} fields.")
                        hpp.tparam(
                            "Ref_Types",
                            "You may optionally provide type filters so that only fields with such types are retrieved.",
                        )
                        hpp.param(
                            "p_Fields",
                            f"A collection of fields. It is advisable for this parameter to be of the type returned by the `Get{modifier}Fields()` methods.",
                        )
                        hpp.param(
                            "p_Fun",
                            "A callable object that must accept any field type. The most straightforward way of doing so is by declaring the macro as `[](const auto &p_Field){};`.",
                        )

                    with hpp.scope(
                        f"template <typename Ref_Type, typename Ref_Fun> static constexpr void ForEach{modifier}Field(const Ref_Type &p_Fields, Ref_Fun &&p_Fun) noexcept"
                    ):
                        with hpp.scope("if constexpr (Iterable<Ref_Type>)", delimiters=False):
                            with hpp.scope(
                                "for (const auto &field : p_Fields)",
                                delimiters=False,
                            ):
                                hpp("std::forward<Ref_Fun>(p_Fun)(field);")
                        with hpp.scope("else", delimiters=False):
                            hpp(
                                "std::apply([&p_Fun](const auto &...p_Field) {(std::forward<Ref_Fun>(p_Fun)(p_Field), ...);}, p_Fields);"
                            )

                    create_get_fields_method(fcollection.fields)
                    create_for_each_method()

                    if fcollection.per_group:
                        with hpp.doc():
                            hpp.brief(f"Get all {modifier.lower()} fields.")
                            hpp.tparam("Ref_Group", "A compile-time group value to filter the retrieved fields.")
                            hpp.tparam(
                                "Ref_Types",
                                "You may optionally provide type filters so that only fields with such types are retrieved.",
                            )
                            hpp.ret(
                                f"If the retrieved fields share all the same type, the return value will be simplified to a `TKit::Array<{modifier}Field>`. If that is not the case however, the fields will be stored in a `std::tuple`."
                            )
                        with hpp.scope(
                            f"template <Group Ref_Group, typename... Ref_Types> static constexpr auto Get{modifier}FieldsByGroup() noexcept"
                        ):
                            cnd = "if"
                            for group in fcollection.per_group:
                                with hpp.scope(
                                    f"{cnd} constexpr (Ref_Group == {modifier}Group::{group})",
                                    delimiters=False,
                                ):
                                    hpp(f"return Get{modifier}{group}Fields<Ref_Types...>();")
                                cnd = "else if"

                            with hpp.scope(
                                "else if constexpr (sizeof...(Ref_Types) == 1)",
                                delimiters=False,
                            ):
                                hpp(f"return Array<{modifier}Field<Ref_Types...>, 0>{{}};")
                            with hpp.scope("else", delimiters=False):
                                hpp("return std::tuple{};")

                        with hpp.doc():
                            hpp.brief(f"Iterate over all {modifier.lower()} fields.")
                            hpp.tparam("Ref_Group", "A compile-time group value to filter the retrieved fields.")
                            hpp.tparam(
                                "Ref_Types",
                                "You may optionally provide type filters so that only fields with such types are retrieved.",
                            )
                            hpp.param(
                                "p_Fun",
                                "A callable object that must accept any field type. The most straightforward way of doing so is by declaring the macro as `[](const auto &p_Field){};`.",
                            )
                        with hpp.scope(
                            f"template <Group Ref_Group, typename... Ref_Types, typename Ref_Fun> static constexpr void ForEach{modifier}FieldByGroup(Ref_Fun &&p_Fun) noexcept"
                        ):
                            hpp(f"const auto fields = Get{modifier}FieldsByGroup<Ref_Group, Ref_Types...>();")
                            hpp(f"ForEach{modifier}Field(fields, std::forward<Ref_Fun>(p_Fun));")

                    for group, fields in fcollection.per_group.items():
                        create_get_fields_method(fields, group=group.name)
                        create_for_each_method(group=group.name)

                    def create_get_tuple_method(*, group: str | None = None) -> None:
                        with hpp.scope(
                            f"template <typename Ref_Type> static constexpr auto get{modifier}{group if group is not None else ''}Tuple() noexcept"
                        ):

                            def generator(fields_cpp: list[str], vtype: str, /) -> None:
                                hpp(f"return {create_tuple_sequence(fields_cpp, vtype=vtype)};")

                            create_if_constexpr_per_type(generator, "std::tuple{}", group=group)

                    def create_get_array_method(*, group: str | None = None) -> None:
                        with hpp.scope(
                            f"template <typename Ref_Type> static constexpr auto get{modifier}{group if group is not None else ''}Array() noexcept"
                        ):

                            def generator(fields_cpp: list[str], vtype: str, /) -> None:
                                hpp(f"return {create_array_sequence(fields_cpp, vtype=vtype)};")

                            create_if_constexpr_per_type(
                                generator,
                                f"Array<{modifier}Field<Ref_Type>, 0>{{}}",
                                group=group,
                            )

                    hpp("private:")
                    create_get_array_method()
                    create_get_tuple_method()
                    for group in fcollection.per_group:
                        create_get_array_method(group=group.name)
                        create_get_tuple_method(group=group.name)

                if memfields.fields:
                    generate_reflect_body(memfields, is_static=False)
                if statfields.fields:
                    generate_reflect_body(statfields, is_static=True)


orchestrator.generate(generate_reflection_code, disclaimer="reflect.py")

Convoy.exit_ok()
