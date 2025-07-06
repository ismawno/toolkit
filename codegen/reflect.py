from cppgen import CPPFile
from argparse import ArgumentParser, Namespace
from pathlib import Path
from collections.abc import Callable
from cpparser import ClassParser, ControlMacros, MacroPair, Field, FieldCollection

import sys

sys.path.append(str(Path(__file__).parent.parent))

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

    parser.add_argument(
        "-i",
        "--input",
        type=Path,
        required=True,
        help="The C++ file to scan for the reflect macro.",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        required=True,
        help="The output file to write the reflection code to.",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="Print more information.",
    )

    return parser.parse_args()


Convoy.log_label = "REFLECT"
args = parse_arguments()
Convoy.is_verbose = args.verbose

output: Path = args.output.resolve()
ffile: Path = args.input.resolve()

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
    gpair,
    ipair,
)

with ffile.open("r") as f:
    content = f.read()
    parser = ClassParser(content, macros=macros)

    if not parser.has_declare_macro():
        Convoy.verbose(f"<fyellow>Macro '{macros.declare}' not found in file '{ffile}'. Exiting...")
        Convoy.exit_ok()
    classes = parser.parse(reserved_group_names="Static")

hpp = CPPFile(output.name)
hpp.disclaimer("reflect.py")
hpp("#pragma once")
hpp.include(str(ffile.resolve()), quotes=True)
hpp.include("tkit/container/array.hpp", quotes=True)
hpp.include("tkit/reflection/reflect.hpp", quotes=True)
hpp.include("tkit/utils/concepts.hpp", quotes=True)
hpp.include("tuple")

with hpp.scope("namespace TKit", indent=0):
    for clsinfo in classes:
        for namespace in clsinfo.namespaces:
            if namespace != "TKit":
                hpp(f"using namespace {namespace};")

        with hpp.doc():
            hpp.brief(
                f"This is an auto-generated specialization of the placeholder `Reflect` class containing compile-time and run-time information about `{clsinfo.name}`."
            )
            hpp(
                "With this specialization, you may query information about the class or struct fields iteratively. If used as a default serialization generation with `TKit::Codec` struct, this file must be included as well before template instantiations of `TKit::Codec` occur."
            )
        with hpp.scope(
            f"template <{clsinfo.template_decl if clsinfo.template_decl is not None else ''}> class Reflect<{clsinfo.name}>",
            closer="};",
        ):
            hpp("public:")
            hpp("static constexpr bool Implemented = true;")
            with hpp.scope("enum class FieldVisibility : u8", closer="};"):
                hpp("Private = 0,")
                hpp("Protected = 1,")
                hpp("Public = 2")

            def generate_reflect_body(fcollection: FieldCollection, /, *, is_static: bool) -> None:
                static = "Static" if is_static else ""

                hpp("public:")
                dtype = "u8" if len(clsinfo.memfields.per_group) < 256 else "u16"
                if fcollection.per_group:
                    with hpp.doc():
                        hpp.brief(
                            f"This enum lists all of the groups defined with the macro pair `{gpair.begin}` and `{gpair.end}`."
                        )
                        hpp("Its values may be used as compile-time filters in certain functions.")

                    with hpp.scope(f"enum class {static}Group : {dtype}", closer="};"):
                        for i, group in enumerate(fcollection.per_group):
                            hpp(f"{group} = {i},")

                with hpp.doc():
                    hpp.brief(f"Encapsulates specific information about a `{clsinfo.name}` field.")
                    hpp("You may access an arbitrary field's value at run-time with the help of this struct.")

                with hpp.scope(f"template <typename Ref_Type> struct {static}Field", closer="};"):
                    hpp("using Type = Ref_Type;")
                    hpp("const char *Name;")
                    hpp("const char *TypeString;")
                    if is_static:
                        hpp("Ref_Type *Pointer;")
                    else:
                        hpp(f"Ref_Type {clsinfo.name}::* Pointer;")
                    hpp("FieldVisibility Visibility;")

                    if not is_static:

                        def create_get_doc() -> None:
                            with hpp.doc():
                                hpp.brief(
                                    f"Given an instance of `{clsinfo.name}`, access the value of the field that is currently represented."
                                )
                                hpp.param("p_Instance", f"An instance of type `{clsinfo.name}`.")
                                hpp.ret(f"The value of the field for the instance.")

                        create_get_doc()
                        with hpp.scope(f"Ref_Type &Get({clsinfo.name} &p_Instance) const noexcept"):
                            hpp("return p_Instance.*Pointer;")

                        create_get_doc()
                        with hpp.scope(f"const Ref_Type &Get(const {clsinfo.name} &p_Instance) const noexcept"):
                            hpp("return p_Instance.*Pointer;")

                        with hpp.doc():
                            hpp.brief(
                                f"Given an instance of `{clsinfo.name}`, set the value of the field that is currently represented."
                            )
                            hpp.param("p_Instance", f"An instance of type `{clsinfo.name}`.")
                            hpp.param("p_Value", "The value to set.")
                        with hpp.scope(
                            f"template <std::convertible_to<Ref_Type> U> void Set({clsinfo.name} &p_Instance, U &&p_Value) const noexcept"
                        ):
                            hpp("p_Instance.*Pointer = std::forward<U>(p_Value);")

                def create_cpp_fields_sequence(fields: list[Field], /, *, group: str | None = None) -> list[str]:
                    def replacer(vtype: str, /) -> str:
                        return vtype.replace('"', r"\"")

                    return [
                        f'{static}Field<{field.vtype}>{{"{field.name}", "{replacer(field.vtype)}", &{clsinfo.name}::{field.name}, FieldVisibility::{field.visibility.capitalize()}}}'
                        for field in fields
                        if group is None or group in field.groups
                    ]

                def create_tuple_sequence(fields: list[str], /, **_) -> str:
                    return f"std::make_tuple({', '.join(fields)})"

                def create_array_sequence(fields: list[str], /, *, vtype: str | None = None) -> str:
                    if vtype is None:
                        vtype = "Ref_Type"
                    return f"Array<{static}Field<{vtype}>, {len(fields)}>{{{', '.join(fields)}}}"

                def create_get_fields_method(fields: list[Field], /, *, group: str = "") -> None:
                    fields_cpp = create_cpp_fields_sequence(fields)

                    with hpp.doc():
                        msg = "Get all " + ("static " if static else "") + "fields"
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
                            "If the retrieved fields share all the same type, the return value will be simplified to a `TKit::Array<Field>`. If that is not the case however, the fields will be stored in a `std::tuple`."
                        )

                    with hpp.scope(
                        f"template <typename... Ref_Types> static constexpr auto Get{static}{group}Fields() noexcept"
                    ):
                        with hpp.scope("if constexpr (sizeof...(Ref_Types) == 0)", delimiters=False):
                            hpp(f"return {create_tuple_sequence(fields_cpp)};")
                        with hpp.scope(
                            "else if constexpr (sizeof...(Ref_Types) == 1)",
                            delimiters=False,
                        ):
                            hpp(f"return get{static}{group}Array<Ref_Types...>();")
                        with hpp.scope("else", delimiters=False):
                            hpp(f"return std::tuple_cat(get{static}{group}Tuple<Ref_Types>()...);")

                def create_for_each_method(*, group: str = "") -> None:
                    with hpp.doc():
                        msg = "Iterate over all " + ("static " if static else "") + "fields"
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
                        f"template <typename... Ref_Types, typename Ref_Fun> static constexpr void ForEach{static}{group}Field(Ref_Fun &&p_Fun) noexcept"
                    ):
                        hpp(f"const auto fields = Get{static}{group}Fields<Ref_Types...>();")
                        hpp(f"ForEach{static}Field(fields, std::forward<Ref_Fun>(p_Fun));")

                with hpp.doc():
                    msg = "Iterate over all " + ("static " if static else "") + "fields."
                    hpp.brief(msg)
                    hpp.tparam(
                        "Ref_Types",
                        "You may optionally provide type filters so that only fields with such types are retrieved.",
                    )
                    hpp.param(
                        "p_Fields",
                        "A collection of fields. It is advisable for this parameter to be of the type returned by the `GetFields()` methods.",
                    )
                    hpp.param(
                        "p_Fun",
                        "A callable object that must accept any field type. The most straightforward way of doing so is by declaring the macro as `[](const auto &p_Field){};`.",
                    )

                with hpp.scope(
                    f"template <typename Ref_Type, typename Ref_Fun> static constexpr void For{static}EachField(const Ref_Type &p_Fields, Ref_Fun &&p_Fun) noexcept"
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

                create_get_fields_method(fcollection.all)
                create_for_each_method()

                if fcollection.per_group:
                    with hpp.doc():
                        msg = "Get all " + ("static " if static else "") + "fields filtered by a specific group enum."
                        hpp.brief(msg)
                        hpp.tparam("Ref_Group", "A compile-time group value to filter the retrieved fields.")
                        hpp.tparam(
                            "Ref_Types",
                            "You may optionally provide type filters so that only fields with such types are retrieved.",
                        )
                        hpp.ret(
                            "If the retrieved fields share all the same type, the return value will be simplified to a `TKit::Array<Field>`. If that is not the case however, the fields will be stored in a `std::tuple`."
                        )
                    with hpp.scope(
                        f"template <Group Ref_Group, typename... Ref_Types> static constexpr auto Get{static}FieldsByGroup() noexcept"
                    ):
                        cnd = "if"
                        for group in fcollection.per_group:
                            with hpp.scope(
                                f"{cnd} constexpr (Ref_Group == {static}Group::{group})",
                                delimiters=False,
                            ):
                                hpp(f"return Get{static}{group}Fields<Ref_Types...>();")
                            cnd = "else if"

                        with hpp.scope(
                            "else if constexpr (sizeof...(Ref_Types) == 1)",
                            delimiters=False,
                        ):
                            hpp(f"return Array<{static}Field<Ref_Types...>, 0>{{}};")
                        with hpp.scope("else", delimiters=False):
                            hpp("return std::tuple{};")

                    with hpp.doc():
                        msg = (
                            "Iterate over all "
                            + ("static " if static else "")
                            + "fields filtered by a specific group enum."
                        )
                        hpp.brief(msg)
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
                        f"template <Group Ref_Group, typename... Ref_Types, typename Ref_Fun> static constexpr void ForEach{static}FieldByGroup(Ref_Fun &&p_Fun) noexcept"
                    ):
                        hpp(f"const auto fields = Get{static}FieldsByGroup<Ref_Group, Ref_Types...>();")
                        hpp(f"ForEach{static}Field(fields, std::forward<Ref_Fun>(p_Fun));")

                for group, fields in fcollection.per_group.items():
                    create_get_fields_method(fields, group=group.name)
                    create_for_each_method(group=group.name)

                def create_if_constexpr_per_type(
                    seq_creator: Callable[..., str],
                    null: str,
                    /,
                    *,
                    group: str | None = None,
                ) -> None:
                    cnd = "if"
                    for vtype, fields in fcollection.per_type.items():
                        fields_cpp = create_cpp_fields_sequence(fields, group=group)
                        if not fields_cpp:
                            continue
                        with hpp.scope(
                            f"{cnd} constexpr (std::is_same_v<Ref_Type, {vtype}>)",
                            delimiters=False,
                        ):
                            hpp(f"return {seq_creator(fields_cpp, vtype=vtype)};")
                        cnd = "else if"
                    if cnd == "else if":
                        with hpp.scope("else", delimiters=False):
                            hpp(f"return {null};")
                    else:
                        hpp(f"return {null};")

                def create_get_tuple_method(*, group: str | None = None) -> None:
                    with hpp.scope(
                        f"template <typename Ref_Type> static constexpr auto get{static}{group if group is not None else ''}Tuple() noexcept"
                    ):
                        create_if_constexpr_per_type(create_tuple_sequence, "std::tuple{}", group=group)

                def create_get_array_method(*, group: str | None = None) -> None:
                    with hpp.scope(
                        f"template <typename Ref_Type> static constexpr auto get{static}{group if group is not None else ''}Array() noexcept"
                    ):
                        create_if_constexpr_per_type(
                            create_array_sequence,
                            f"Array<{static}Field<Ref_Type>, 0>{{}}",
                            group=group,
                        )

                hpp("private:")
                create_get_array_method()
                create_get_tuple_method()
                for group in fcollection.per_group:
                    create_get_array_method(group=group.name)
                    create_get_tuple_method(group=group.name)

            if clsinfo.memfields.all:
                generate_reflect_body(clsinfo.memfields, is_static=False)
            if clsinfo.statfields.all:
                generate_reflect_body(clsinfo.statfields, is_static=True)

hpp.write(output.parent)

Convoy.exit_ok()
