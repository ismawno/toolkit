from cppgen import CPPFile
from argparse import ArgumentParser, Namespace
from pathlib import Path
from dataclasses import dataclass
from collections.abc import Callable

import re


def parse_arguments() -> Namespace:

    desc = """
    This python script takes in a c++ file and scans it for classes/structs marked with a
    special reflect macro (specified with the --macro option). If it finds any, it will generate
    another c++ file containing a template specialization of the 'Reflect' class (can be specified as well),
    which will contain all sorts of compile and runtime reflection information about the class/struct.
    """
    parser = ArgumentParser(description=desc)

    parser.add_argument(
        "-i",
        "--input",
        type=Path,
        required=True,
        help="The c++ file to scan for the reflect macro.",
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
    privacy: str
    vtype: str
    is_static: bool
    groups: set[str]

    def as_str(self, parent: str, /) -> str:
        sct = "static" if self.is_static else "non-static"
        return f"{sct} {self.privacy} {self.vtype} {parent}::{self.name}"


@dataclass(frozen=True)
class ClassInfo:
    name: str
    namespaces: list[str]
    fields: list[Field]
    fields_per_type: dict[str, list[Field]]
    fields_per_group: dict[str, list[Field]]


def parse_class(
    macro: Macro, lines: list[str], clsname: str, namespaces: list[str], /
) -> ClassInfo:
    name = re.match(rf"{clsname} ([a-zA-Z0-9_]+)", lines[0]).group(1)
    privacy = "private" if clsname == "class" else "public"

    groups = []
    fields = []
    fields_per_type = {}
    fields_per_group = {}

    scope_counter = 0
    ignore = False
    for line in lines:
        line = line.strip()
        if macro.group_begin in line:
            group = (
                re.match(rf"{macro.group_begin}\((.*?)\)", line)
                .group(1)
                .replace('"', "")
            )
            if group == "":
                raise ValueError("Group name cannot be empty")
            if group == "Group":
                raise ValueError("Group name cannot be 'Group'. It is a reserved name")
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
            raise ValueError(
                f"Scope counter reached a negative value!: {scope_counter}"
            )
        if scope_counter != 1:
            continue

        def check_privacy(look_for: str, /) -> bool:
            nonlocal privacy
            if f"{look_for}:" in line:
                privacy = look_for

        check_privacy("private")
        check_privacy("public")
        check_privacy("protected")

        if not line.endswith(";"):
            continue
        line = line.replace(";", "").strip().removeprefix("inline ")
        if "=" not in line and "{" not in line and "(" in line:
            continue

        is_static = line.startswith("static")
        line = line.removeprefix("static ").removeprefix("inline ")

        line = re.sub(r"=.*", "", line)
        line = re.sub(r"{.*", "", line).strip().replace(", ", ",")

        # Wont work for members written like int*x. Must be int* x or int *x
        vtype, vname = line.split(" ", 1)
        if "*" in vname:
            vtype = f"{vtype}*"
            vname = vname.replace("*", "")
        if "&" in vname:
            vtype = f"{vtype}&"
            vname = vname.replace("&", "")

        field = Field(
            vname,
            privacy,
            vtype.replace(",", ", "),
            is_static,
            set(groups),
        )
        fields.append(field)
        fields_per_type.setdefault(field.vtype, []).append(field)
        for group in groups:
            fields_per_group.setdefault(group, []).append(field)

    if ignore:
        raise ValueError("Ignore macro was not closed properly")

    if groups:
        raise ValueError("Group macro was not closed properly")

    return ClassInfo(name, namespaces, fields, fields_per_type, fields_per_group)


def parse_classes_in_file(
    lines: list[str],
    log: Callable[[str], None],
    macro: Macro,
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

        clsname = "class" if is_class else "struct"

        clinfo = parse_class(macro, sublines, clsname, namespaces)

        log(f"Found '{clinfo.name}' {clsname}. Parsing...")
        for field in clinfo.fields:
            log(f"  Successfully registered field member '{field.as_str(clinfo.name)}'")
        classes.append(clinfo)

    return classes


def main() -> None:
    args = parse_arguments()
    output: Path = args.output
    ffile: Path = args.input
    macro = Macro(
        args.declare_macro,
        f"{args.ignore_macro}_BEGIN",
        f"{args.ignore_macro}_END",
        f"{args.group_macro}_BEGIN",
        f"{args.group_macro}_END",
    )

    def log(*pargs, **kwargs) -> None:
        if args.verbose:
            print(*pargs, **kwargs)

    # if output.exists() and output.stat().st_mtime > ffile.stat().st_mtime:
    #     log(f"Output file '{output}' is newer than input file '{ffile}'. Exiting...")
    #     return

    path = output.parent

    with ffile.open("r") as f:

        def remove_comments(text: str, /) -> str:
            text = re.sub(r"//.*", "", text)
            return re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)

        content = (
            remove_comments(f.read())
            .replace("<class", "<typename")
            .replace(", class", ", typename")
        )
        if macro.declare not in content:
            log(f"Macro '{macro.declare}' not found in file '{ffile}'. Exiting...")
            return
        classes = parse_classes_in_file(content.splitlines(), log, macro)

    cpp = CPPFile(args.output.name)
    cpp(f'#include "{ffile.resolve()}"')
    cpp('#include "tkit/container/array.hpp"')
    cpp('#include "tkit/reflection/reflect.hpp"')
    cpp('#include "tkit/utils/concepts.hpp"')
    cpp(f"#include <tuple>")

    with cpp.scope("namespace TKit", indent=False):
        for cls in classes:
            for namespace in cls.namespaces:
                if namespace != "TKit":
                    cpp(f"using namespace {namespace};")

            with cpp.scope(f"template <> class Reflect<{cls.name}>", semicolon=True):
                cpp("public:")
                cpp("static constexpr bool Implemented = true;")
                dtype = "u8" if len(cls.fields_per_group) < 256 else "u16"
                with cpp.scope(f"enum class Group : {dtype}", semicolon=True):
                    for i, group in enumerate(cls.fields_per_group):
                        cpp(f"{group} = {i},")

                with cpp.scope("template <typename T> struct Field", semicolon=True):
                    cpp("using Type = T;")
                    cpp("const char *Name;")
                    cpp(f"T {cls.name}::* Pointer;")

                    with cpp.scope(f"T &Get({cls.name} &p_Instance) const noexcept"):
                        cpp("return p_Instance.*Pointer;")
                    with cpp.scope(
                        f"const T &Get(const {cls.name} &p_Instance) const noexcept"
                    ):
                        cpp("return p_Instance.*Pointer;")

                    with cpp.scope(
                        f"template <std::convertible_to<T> U> void Set({cls.name} &p_Instance, U &&p_Value) const noexcept"
                    ):
                        cpp("p_Instance.*Pointer = std::forward<U>(p_Value);")

                def create_cpp_fields_sequence(
                    fields: list[Field], /, *, group: str | None = None
                ) -> list[str]:
                    return [
                        f'Field<{field.vtype}>{{"{field.name}", &{cls.name}::{field.name}}}'
                        for field in fields
                        if (not args.exclude_non_public or field.privacy == "public")
                        and (group is None or group in field.groups)
                    ]

                def create_tuple_sequence(fields: list[str], /, **_) -> str:
                    return f"std::make_tuple({', '.join(fields)})"

                def create_array_sequence(
                    fields: list[str], /, *, vtype: str | None = None
                ) -> str:
                    if vtype is None:
                        vtype = "T"
                    return (
                        f"Array<Field<{vtype}>, {len(fields)}>{{{', '.join(fields)}}}"
                    )

                def create_get_fields_method(
                    fields: list[str], /, *, group: str = ""
                ) -> None:
                    fields_cpp = create_cpp_fields_sequence(fields)
                    if not fields_cpp:
                        return
                    with cpp.scope(
                        f"template <typename... Args> static constexpr auto Get{group}Fields() noexcept"
                    ):
                        with cpp.scope(
                            "if constexpr (sizeof...(Args) == 0)", curlies=False
                        ):
                            cpp(f"return {create_tuple_sequence(fields_cpp)};")
                        with cpp.scope(
                            "else if constexpr (sizeof...(Args) == 1)", curlies=False
                        ):
                            cpp(f"return get{group}Array<Args...>();")
                        with cpp.scope("else", curlies=False):
                            cpp(f"return std::tuple_cat(get{group}Tuple<Args>()...);")

                def create_for_each_method(*, group: str = "") -> None:
                    with cpp.scope(
                        f"template <typename... Args, typename F> static constexpr void ForEach{group}Field(F &&p_Fun) noexcept"
                    ):
                        cpp(f"const auto fields = Get{group}Fields<Args...>();")
                        cpp("ForEachField(fields, std::forward<F>(p_Fun));")

                with cpp.scope(
                    "template <typename T, typename F> static constexpr void ForEachField(T &&p_Fields, F &&p_Fun) noexcept"
                ):
                    with cpp.scope("if constexpr (Iterable<T>)", curlies=False):
                        with cpp.scope(
                            "for (const auto &field : std::forward<T>(p_Fields))",
                            curlies=False,
                        ):
                            cpp("std::forward<F>(p_Fun)(field);")
                    with cpp.scope("else", curlies=False):
                        cpp(
                            "std::apply([&p_Fun](const auto &...p_Field) {(std::forward<F>(p_Fun)(p_Field), ...);}, std::forward<T>(p_Fields));"
                        )

                create_get_fields_method(cls.fields)
                create_for_each_method()

                if cls.fields_per_group:
                    with cpp.scope(
                        "template <Group G, typename... Args> static constexpr auto GetFieldsByGroup() noexcept"
                    ):
                        cnd = "if"
                        for group in cls.fields_per_group:
                            with cpp.scope(
                                f"{cnd} constexpr (G == Group::{group})", curlies=False
                            ):
                                cpp(f"return Get{group}Fields<Args...>();")
                            cnd = "else if"

                        with cpp.scope(
                            "else if constexpr (sizeof...(Args) == 1)", curlies=False
                        ):
                            cpp("return Array<Field<Args...>, 0>{};")
                        with cpp.scope("else", curlies=False):
                            cpp("return std::tuple{};")

                    with cpp.scope(
                        "template <Group G, typename... Args, typename F> static constexpr void ForEachFieldByGroup(F &&p_Fun) noexcept"
                    ):
                        cpp("const auto fields = GetFieldsByGroup<G, Args...>();")
                        cpp("ForEachField(fields, std::forward<F>(p_Fun));")

                for group, fields in cls.fields_per_group.items():
                    create_get_fields_method(fields, group=group)
                    create_for_each_method(group=group)

                def create_if_constexpr_per_type(
                    seq_creator: Callable[[list[str]], str],
                    null: str,
                    /,
                    *,
                    group: str | None = None,
                ) -> None:
                    cnd = "if"
                    for vtype, fields in cls.fields_per_type.items():
                        fields_cpp = create_cpp_fields_sequence(fields, group=group)
                        if not fields_cpp:
                            continue
                        with cpp.scope(
                            f"{cnd} constexpr (std::is_same_v<T, {vtype}>)",
                            curlies=False,
                        ):
                            cpp(f"return {seq_creator(fields_cpp, vtype=vtype)};")
                        cnd = "else if"
                    if cnd == "else if":
                        with cpp.scope("else", curlies=False):
                            cpp(f"return {null};")
                    else:
                        cpp(f"return {null};")

                def crate_get_tuple_method(*, group: str | None = None) -> None:
                    with cpp.scope(
                        f"template <typename T> static constexpr auto get{group if group is not None else ''}Tuple() noexcept"
                    ):
                        create_if_constexpr_per_type(
                            create_tuple_sequence, "std::tuple{}", group=group
                        )

                def crate_get_array_method(*, group: str | None = None) -> None:
                    with cpp.scope(
                        f"template <typename T> static constexpr auto get{group if group is not None else ''}Array() noexcept"
                    ):
                        create_if_constexpr_per_type(
                            create_array_sequence, "Array<Field<T>, 0>{}", group=group
                        )

                cpp("private:")
                crate_get_array_method()
                crate_get_tuple_method()
                for group in cls.fields_per_group:
                    crate_get_array_method(group=group)
                    crate_get_tuple_method(group=group)

    cpp.write(path)


if __name__ == "__main__":
    main()
