from cppgen import CPPFile
from argparse import ArgumentParser, Namespace
from pathlib import Path
from dataclasses import dataclass
from collections.abc import Callable

import sys
import re

sys.path.append(str(Path(__file__).parent.parent))

from convoy import Convoy


def parse_arguments() -> Namespace:
    desc = """
    This python script takes in a C++ file and scans it for classes/structs marked with a
    special reflect macro (specified with the '--macro' option). If it finds any, it will generate
    another C++ file containing a template specialization of a special reflect class (specified
    with the '--class-name' option) which will contain compile and run time reflection
    information about the class/struct's fields.
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
        "-c",
        "--class-name",
        type=str,
        default="Reflect",
        help="The name of the template specialization class. Default is 'Reflect'.",
    )
    parser.add_argument(
        "--declare-macro",
        type=str,
        default="TKIT_REFLECT_DECLARE",
        help="The macro that will mark a specific class or struct for reflection. Default is 'TKIT_REFLECT_DECLARE'.",
    )
    parser.add_argument(
        "--ignore-macro",
        type=str,
        default="TKIT_REFLECT_IGNORE",
        help="The the macro prefix that will mark some fields of a marked class or struct to be ignored. This reflection script will look for <macro>_BEGIN and <macro>_END. Default is 'TKIT_REFLECT_IGNORE'.",
    )
    parser.add_argument(
        "--group-macro",
        type=str,
        default="TKIT_REFLECT_GROUP",
        help="The macro prefix that will group some fields of a marked class or struct. This reflection script will look for <macro>_BEGIN and <macro>_END. Default is 'TKIT_REFLECT_GROUP'.",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="Print more information.",
    )
    parser.add_argument(
        "--exclude-non-public",
        action="store_true",
        default=False,
        help="Exclude private and protected data members when reflecting.",
    )

    return parser.parse_args()


@dataclass(frozen=True)
class Macro:
    declare: str
    ignore_begin: str
    ignore_end: str
    group_begin: str
    group_end: str


@dataclass(frozen=True)
class Field:
    name: str
    visibility: str
    vtype: str
    groups: set[str]

    def as_str(self, parent: str, /, *, is_static: bool) -> str:
        sct = "static" if is_static else "non-static"
        return f"{sct} {self.visibility} {self.vtype} {parent}::{self.name}"


@dataclass(frozen=True)
class FieldCollection:
    all: list[Field]
    per_type: dict[str, list[Field]]
    per_group: dict[str, list[Field]]


@dataclass(frozen=True)
class ClassInfo:
    name: str
    namespaces: list[str]
    nstatic: FieldCollection
    static: FieldCollection
    template_decl: str | None


def parse_class(
    lines: list[str],
    template_line: str | None,
    clstype: str,
    namespaces: list[str],
    /,
) -> ClassInfo:
    clsline = lines[0].replace("template ", "template")
    name = re.match(rf".*{clstype} ([a-zA-Z0-9_<>, ]+)", clsline)
    if name is not None:
        name = name.group(1)
    else:
        Convoy.exit_error(f"A match was not found when trying to extract the name of the {clstype}.")
    visibility = "private" if clstype == "class" else "public"

    mtch = re.match(r".*template<(.*?)>", clsline)
    template_decl = mtch.group(1) if mtch is not None else None

    if template_decl is not None and template_line is not None:
        Convoy.exit_error("Found duplicate template line.")

    if template_decl is None and template_line is not None:
        template_decl = re.match(r".*template<(.*?)>", template_line)
        if template_decl is not None:
            template_decl = template_decl.group(1).replace(" ,", ",")
        else:
            Convoy.exit_error(
                f"A match was not found when trying to extract the template declaration of the {name} {clstype}."
            )

    if template_decl is not None and "<" not in name:
        template_vars = ", ".join([var.split(" ")[1] for var in template_decl.replace(", ", ",").split(",")])
        name = f"{name}<{template_vars}>"

    groups = []

    nstatic_fields = []
    nstatic_fields_per_type = {}
    nstatic_fields_per_group = {}

    static_fields = []
    static_fields_per_type = {}
    static_fields_per_group = {}

    scope_counter = 0
    ignore = False
    for line in lines:
        line = line.strip()
        if macro.group_begin in line:
            group = re.match(rf"{macro.group_begin}\((.*?)\)", line)
            if group is not None:
                group = group.group(1).replace('"', "")
            else:
                Convoy.exit_error(f"Failed to match group name macro")
            if group == "":
                Convoy.exit_error("Group name cannot be empty.")
            if group == "Static":
                Convoy.exit_error("Group name cannot be <bold>Static</bold>. It is a reserved name.")
            groups.append(group)

        elif macro.group_end in line:
            groups.pop()

        if macro.ignore_end in line:
            ignore = False
        elif ignore or macro.ignore_begin in line:
            ignore = True
            continue

        if line == "};":
            break

        if "{" in line:
            scope_counter += 1

        if "}" in line:
            scope_counter -= 1

        if scope_counter < 0:
            Convoy.exit_error(f"Scope counter reached a negative value: {scope_counter}.")
        if scope_counter != 1:
            continue

        def check_privacy(look_for: str, /) -> None:
            nonlocal visibility
            if f"{look_for}:" in line:
                visibility = look_for

        check_privacy("private")
        check_privacy("public")
        check_privacy("protected")
        if visibility != "public" and args.exclude_non_public:
            continue

        if not line.endswith(";") or "noexcept" in line:
            continue

        line = line.replace(";", "").strip().removeprefix("inline ")
        if "=" not in line and "{" not in line and ("(" in line or ")" in line):
            continue

        is_static = line.startswith("static")
        line = line.removeprefix("static ").removeprefix("inline ")

        line = re.sub(r"=.*", "", line)
        line = re.sub(r"{.*", "", line).strip().replace(", ", ",")

        splits = line.split(" ")
        ln = len(splits) - ("const" in line)
        if ln < 2:
            continue

        # Wont work for members written like int*x. Must be int* x or int *x
        vtype, vname = splits[:2]
        if "*" in vname:
            vtype = f"{vtype}*"
            vname = vname.replace("*", "")
        if "&" in vname:
            vtype = f"{vtype}&"
            vname = vname.replace("&", "")

        field = Field(
            vname,
            visibility,
            vtype.replace(",", ", "),
            set(groups),
        )
        fields = static_fields if is_static else nstatic_fields
        fields_per_type = static_fields_per_type if is_static else nstatic_fields_per_type
        fields_per_group = static_fields_per_group if is_static else nstatic_fields_per_group

        fields.append(field)
        fields_per_type.setdefault(field.vtype, []).append(field)
        for group in groups:
            fields_per_group.setdefault(group, []).append(field)

    if ignore:
        Convoy.exit_error(f"Ignore macro was not closed properly with a <bold>{macro.ignore_end}</bold>.")

    if groups:
        Convoy.exit_error(f"Group macro was not closed properly with a <bold>{macro.group_end}</bold>.")

    return ClassInfo(
        name,
        namespaces,
        FieldCollection(nstatic_fields, nstatic_fields_per_type, nstatic_fields_per_group),
        FieldCollection(static_fields, static_fields_per_type, static_fields_per_group),
        template_decl,
    )


def parse_classes_in_file(
    lines: list[str],
    /,
) -> list[ClassInfo]:

    classes = []
    namespaces = []
    for i, line in enumerate(lines):
        if line.endswith(";"):
            continue

        match = re.match(r"namespace ([a-zA-Z0-9_::]+)", line)
        if match is not None:
            namespace = match.group(1).split("::")
            namespaces.extend(namespace)
            continue

        is_class = "class" in line
        is_struct = not is_class and "struct" in line

        if not is_class and not is_struct:
            continue

        sublines = lines[i:]
        for subline in sublines:
            found_macro = macro.declare in subline
            found_end = subline.endswith("};")
            if found_macro or found_end:
                break
        else:
            continue

        if found_end:
            continue

        template_line = (
            lines[i - 1]
            if i > 0 and "template" in lines[i - 1] and "struct" not in lines[i - 1] and "class" not in lines[i - 1]
            else None
        )
        clstype = "class" if is_class else "struct"
        clinfo = parse_class(sublines, template_line, clstype, namespaces)

        Convoy.verbose(f"Found and parsed <bold>{clinfo.name}</bold> {clstype}.")
        for field in clinfo.nstatic.all:
            Convoy.verbose(f"  -Registered field member <bold>{field.as_str(clinfo.name, is_static=False)}</bold>.")
        for field in clinfo.static.all:
            Convoy.verbose(f"  -Registered field member <bold>{field.as_str(clinfo.name, is_static=True)}</bold>.")
        classes.append(clinfo)

    return classes


Convoy.log_label = "REFLECT"
args = parse_arguments()
Convoy.is_verbose = args.verbose

output: Path = args.output.resolve()
ffile: Path = args.input.resolve()
macro = Macro(
    args.declare_macro,
    f"{args.ignore_macro}_BEGIN",
    f"{args.ignore_macro}_END",
    f"{args.group_macro}_BEGIN",
    f"{args.group_macro}_END",
)

with ffile.open("r") as f:

    def remove_comments(text: str, /) -> str:
        text = re.sub(r"//.*", "", text)
        return re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)

    content = remove_comments(f.read()).replace("<class", "<typename").replace(", class", ", typename")
    if macro.declare not in content:
        Convoy.verbose(f"<fyellow>Macro '{macro.declare}' not found in file '{ffile}'. Exiting...")
        sys.exit()
    classes = parse_classes_in_file(content.splitlines())

hpp = CPPFile(output.name)
hpp.disclaimer("reflect.py")
hpp("#pragma once")
hpp.include(str(ffile.resolve()), quotes=True)
hpp.include("tkit/container/array.hpp", quotes=True)
hpp.include("tkit/reflection/reflect.hpp", quotes=True)
hpp.include("tkit/utils/concepts.hpp", quotes=True)
hpp.include("tuple")

with hpp.scope("namespace TKit", indent=0):
    for cls in classes:
        for namespace in cls.namespaces:
            if namespace != "TKit":
                hpp(f"using namespace {namespace};")

        with hpp.scope(
            f"template <{cls.template_decl if cls.template_decl is not None else ''}> class Reflect<{cls.name}>",
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
                dtype = "u8" if len(cls.nstatic.per_group) < 256 else "u16"
                if fcollection.per_group:
                    with hpp.scope(f"enum class {static}Group : {dtype}", closer="};"):
                        for i, group in enumerate(fcollection.per_group):
                            hpp(f"{group} = {i},")

                with hpp.scope(f"template <typename T> struct {static}Field", closer="};"):
                    hpp("using Type = T;")
                    hpp("const char *Name;")
                    hpp("const char *TypeString;")
                    if is_static:
                        hpp("T *Pointer;")
                    else:
                        hpp(f"T {cls.name}::* Pointer;")
                    hpp("FieldVisibility Visibility;")

                    if not is_static:
                        with hpp.scope(f"T &Get({cls.name} &p_Instance) const noexcept"):
                            hpp("return p_Instance.*Pointer;")
                        with hpp.scope(f"const T &Get(const {cls.name} &p_Instance) const noexcept"):
                            hpp("return p_Instance.*Pointer;")

                        with hpp.scope(
                            f"template <std::convertible_to<T> U> void Set({cls.name} &p_Instance, U &&p_Value) const noexcept"
                        ):
                            hpp("p_Instance.*Pointer = std::forward<U>(p_Value);")

                def create_cpp_fields_sequence(fields: list[Field], /, *, group: str | None = None) -> list[str]:
                    def replacer(vtype: str, /) -> str:
                        return vtype.replace('"', r"\"")

                    return [
                        f'{static}Field<{field.vtype}>{{"{field.name}", "{replacer(field.vtype)}", &{cls.name}::{field.name}, FieldVisibility::{field.visibility.capitalize()}}}'
                        for field in fields
                        if group is None or group in field.groups
                    ]

                def create_tuple_sequence(fields: list[str], /, **_) -> str:
                    return f"std::make_tuple({', '.join(fields)})"

                def create_array_sequence(fields: list[str], /, *, vtype: str | None = None) -> str:
                    if vtype is None:
                        vtype = "T"
                    return f"Array<{static}Field<{vtype}>, {len(fields)}>{{{', '.join(fields)}}}"

                def create_get_fields_method(fields: list[Field], /, *, group: str = "") -> None:
                    fields_cpp = create_cpp_fields_sequence(fields)
                    with hpp.scope(
                        f"template <typename... Args> static constexpr auto Get{static}{group}Fields() noexcept"
                    ):
                        with hpp.scope("if constexpr (sizeof...(Args) == 0)", delimiters=False):
                            hpp(f"return {create_tuple_sequence(fields_cpp)};")
                        with hpp.scope(
                            "else if constexpr (sizeof...(Args) == 1)",
                            delimiters=False,
                        ):
                            hpp(f"return get{static}{group}Array<Args...>();")
                        with hpp.scope("else", delimiters=False):
                            hpp(f"return std::tuple_cat(get{static}{group}Tuple<Args>()...);")

                def create_for_each_method(*, group: str = "") -> None:
                    with hpp.scope(
                        f"template <typename... Args, typename F> static constexpr void ForEach{static}{group}Field(F &&p_Fun) noexcept"
                    ):
                        hpp(f"const auto fields = Get{static}{group}Fields<Args...>();")
                        hpp(f"ForEach{static}Field(fields, std::forward<F>(p_Fun));")

                with hpp.scope(
                    f"template <typename T, typename F> static constexpr void For{static}EachField(const T &p_Fields, F &&p_Fun) noexcept"
                ):
                    with hpp.scope("if constexpr (Iterable<T>)", delimiters=False):
                        with hpp.scope(
                            "for (const auto &field : p_Fields)",
                            delimiters=False,
                        ):
                            hpp("std::forward<F>(p_Fun)(field);")
                    with hpp.scope("else", delimiters=False):
                        hpp(
                            "std::apply([&p_Fun](const auto &...p_Field) {(std::forward<F>(p_Fun)(p_Field), ...);}, p_Fields);"
                        )

                create_get_fields_method(fcollection.all)
                create_for_each_method()

                if fcollection.per_group:
                    with hpp.scope(
                        f"template <Group G, typename... Args> static constexpr auto Get{static}FieldsByGroup() noexcept"
                    ):
                        cnd = "if"
                        for group in fcollection.per_group:
                            with hpp.scope(
                                f"{cnd} constexpr (G == {static}Group::{group})",
                                delimiters=False,
                            ):
                                hpp(f"return Get{static}{group}Fields<Args...>();")
                            cnd = "else if"

                        with hpp.scope(
                            "else if constexpr (sizeof...(Args) == 1)",
                            delimiters=False,
                        ):
                            hpp(f"return Array<{static}Field<Args...>, 0>{{}};")
                        with hpp.scope("else", delimiters=False):
                            hpp("return std::tuple{};")

                    with hpp.scope(
                        f"template <Group G, typename... Args, typename F> static constexpr void ForEach{static}FieldByGroup(F &&p_Fun) noexcept"
                    ):
                        hpp(f"const auto fields = Get{static}FieldsByGroup<G, Args...>();")
                        hpp(f"ForEach{static}Field(fields, std::forward<F>(p_Fun));")

                for group, fields in fcollection.per_group.items():
                    create_get_fields_method(fields, group=group)
                    create_for_each_method(group=group)

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
                            f"{cnd} constexpr (std::is_same_v<T, {vtype}>)",
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
                        f"template <typename T> static constexpr auto get{static}{group if group is not None else ''}Tuple() noexcept"
                    ):
                        create_if_constexpr_per_type(create_tuple_sequence, "std::tuple{}", group=group)

                def create_get_array_method(*, group: str | None = None) -> None:
                    with hpp.scope(
                        f"template <typename T> static constexpr auto get{static}{group if group is not None else ''}Array() noexcept"
                    ):
                        create_if_constexpr_per_type(
                            create_array_sequence,
                            f"Array<{static}Field<T>, 0>{{}}",
                            group=group,
                        )

                hpp("private:")
                create_get_array_method()
                create_get_tuple_method()
                for group in fcollection.per_group:
                    create_get_array_method(group=group)
                    create_get_tuple_method(group=group)

            generate_reflect_body(cls.nstatic, is_static=False)
            generate_reflect_body(cls.static, is_static=True)

hpp.write(output.parent)

Convoy.exit_ok()
