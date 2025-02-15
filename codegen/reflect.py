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
        "-f",
        "--file",
        type=Path,
        required=True,
        help="The c++ file to scan for the reflect macro.",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=Path("reflect.cpp"),
        help="The output file to write the reflection code to.",
    )
    parser.add_argument(
        "-c",
        "--class-name",
        type=str,
        default="Reflect",
        help="The name of the template specialization class.",
    )
    parser.add_argument(
        "-m",
        "--macro",
        type=str,
        default="TKIT_REFLECT",
        help="The macro to look for in the c++ file.",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="Print more information.",
    )
    parser.add_argument(
        "--include-non-public",
        action="store_true",
        default=False,
        help="Include private and protected data members when reflecting.",
    )

    return parser.parse_args()


@dataclass(frozen=True)
class Field:
    name: str
    privacy: str
    vtype: str
    is_static: bool

    def as_str(self, parent: str, /) -> str:
        sct = "static" if self.is_static else "non-static"
        return f"{sct} {self.privacy} {self.vtype} {parent}::{self.name}"


@dataclass(frozen=True)
class ClassInfo:
    name: str
    namespaces: list[str]
    fields: list[Field]
    fields_per_type: dict[str, list[Field]]


def parse_class(lines: list[str], clsname: str, namespaces: list[str], /) -> ClassInfo:
    name = re.match(rf"{clsname} ([a-zA-Z0-9_]+)", lines[0]).group(1)
    privacy = "private" if clsname == "class" else "public"

    fields = []
    fields_per_type = {}
    scope_counter = 0
    for line in lines:
        line = line.strip()
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

        field = Field(vname, privacy, vtype.replace(",", ", "), is_static)
        fields.append(field)
        fields_per_type.setdefault(field.vtype, []).append(field)

    return ClassInfo(name, namespaces, fields, fields_per_type)


def parse_classes_in_file(
    lines: list[str],
    log: Callable[[str], None],
    macro: str,
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
            found_macro = macro in subline
            found_end = subline.endswith("};")
            if found_macro or found_end:
                break
        else:
            continue

        if found_end:
            continue

        clsname = "class" if is_class else "struct"

        clinfo = parse_class(sublines, clsname, namespaces)

        log(f"Found and parsed '{clinfo.name}' {clsname}")
        for field in clinfo.fields:
            log(f"  Successfully registered field member '{field.as_str(clinfo.name)}'")
        classes.append(clinfo)

    return classes


def main() -> None:
    args = parse_arguments()
    output: Path = args.output
    ffile: Path = args.file
    macro: str = args.macro

    def log(*pargs, **kwargs) -> None:
        if args.verbose:
            print(*pargs, **kwargs)

    if output.exists() and output.stat().st_mtime > ffile.stat().st_mtime:
        log(f"Output file '{output}' is newer than input file '{ffile}'. Exiting...")
        return

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
        if macro not in content:
            log(f"Macro '{macro}' not found in file '{ffile}'. Exiting...")
            return
        classes = parse_classes_in_file(content.splitlines(), log, macro)

    cpp = CPPFile(args.output.name)
    cpp(f'#include "{ffile.resolve()}"')
    cpp('#include "tkit/container/array.hpp"')
    cpp('#include "tkit/reflection/reflect.hpp"')
    cpp(f"#include <tuple>")

    with cpp.scope("namespace TKit", indent=False):
        for cls in classes:
            for namespace in cls.namespaces:
                if namespace != "TKit":
                    cpp(f"using namespace {namespace};")

            with cpp.scope(f"template <> class Reflect<{cls.name}>", semicolon=True):
                cpp("public:")
                cpp("static constexpr bool Implemented = true;")
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

                def create_cpp_fields_sequence(fields: list[Field], /) -> list[str]:
                    return [
                        f'Field<{field.vtype}>{{"{field.name}", &{cls.name}::{field.name}}}'
                        for field in fields
                        if args.include_non_public or field.privacy == "public"
                    ]

                def create_tuple_sequence(fields: list[str], /, **_) -> str:
                    return f"std::make_tuple({', '.join(fields)})"

                def create_array_sequence(
                    fields: list[str], *, vtype: str | None = None
                ) -> str:
                    if vtype is None:
                        vtype = "T"
                    return (
                        f"Array<Field<{vtype}>, {len(fields)}>{{{', '.join(fields)}}}"
                    )

                def create_if_constexpr_per_type(
                    seq_creator: Callable[[list[str]], str], null: str, /
                ) -> None:
                    cnd = "if"
                    for vtype, fields in cls.fields_per_type.items():
                        fields_cpp = create_cpp_fields_sequence(fields)
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

                fields_cpp = create_cpp_fields_sequence(cls.fields)
                if fields_cpp:
                    with cpp.scope(
                        "template <typename ...Args> static constexpr auto GetFields() noexcept"
                    ):
                        with cpp.scope(
                            "if constexpr (sizeof...(Args) == 0)", curlies=False
                        ):
                            cpp(f"return {create_tuple_sequence(fields_cpp)};")
                        with cpp.scope(
                            "else if constexpr (sizeof...(Args) == 1)", curlies=False
                        ):
                            cpp(f"return getArray<Args...>();")
                        with cpp.scope("else", curlies=False):
                            cpp("return std::tuple_cat(getTuple<Args>()...);")

                with cpp.scope(
                    "template <typename...Args, typename F> static constexpr auto ForEachField(F &&p_Fun) noexcept"
                ):
                    cpp("const auto fields = GetFields<Args...>();")
                    with cpp.scope(
                        "if constexpr (sizeof...(Args) == 1)", curlies=False
                    ):
                        with cpp.scope(
                            "for (const auto &field : fields)", curlies=False
                        ):
                            cpp("std::forward<F>(p_Fun)(field);")
                    with cpp.scope("else", curlies=False):
                        cpp(
                            "std::apply([&p_Fun](const auto &...p_Field) {(std::forward<F>(p_Fun)(p_Field), ...);}, fields);"
                        )

                # Let Parse take a span as well

                cpp("private:")
                with cpp.scope(
                    "template <typename T> static constexpr auto getTuple() noexcept"
                ):
                    create_if_constexpr_per_type(create_tuple_sequence, "std::tuple{}")

                with cpp.scope(
                    "template <typename T> static constexpr auto getArray() noexcept"
                ):
                    create_if_constexpr_per_type(
                        create_array_sequence, "Array<Field<T>, 0>{}"
                    )

    with cpp.scope("int main()"):
        cpp("return 0;")

    cpp.write(path)


if __name__ == "__main__":
    main()
