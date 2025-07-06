from dataclasses import dataclass
from pathlib import Path

import sys
import re

sys.path.append(str(Path(__file__).parent.parent))

from convoy import Convoy


@dataclass(frozen=True)
class MacroPair:
    begin: str
    end: str


@dataclass(frozen=True)
class ControlMacros:
    declare: str | None = None
    group: MacroPair | None = None
    ignore: MacroPair | None = None


@dataclass(frozen=True)
class Group:
    name: str
    properties: list[str]

    def __eq__(self, other) -> bool:
        if isinstance(other, str):
            return self.name == other
        if isinstance(other, Group):
            return self.name == other.name

        return NotImplemented

    def __hash__(self):
        return hash(self.name)

    def __str__(self) -> str:
        return self.name


@dataclass(frozen=True)
class Field:
    name: str
    visibility: str
    vtype: str
    groups: list[Group]

    def as_str(self, parent: str, /, *, is_static: bool) -> str:
        sct = "static" if is_static else "non-static"
        return f"{sct} {self.visibility} {self.vtype} {parent}::{self.name}"


@dataclass(frozen=True)
class FieldCollection:
    all: list[Field]
    per_type: dict[str, list[Field]]
    per_group: dict[Group, list[Field]]


@dataclass(frozen=True)
class Class:
    name: str
    namespaces: list[str]
    memfields: FieldCollection
    statfields: FieldCollection
    template_decl: str | None


class ClassParser:

    def __init__(self, code: str, /, *, macros: ControlMacros | None = None) -> None:
        self.__code = (
            code.replace("<class", "<typename").replace(", class", ", typename").replace("template ", "template")
        )
        self.__macros = macros

    def has_declare_macro(self) -> bool:
        if self.__macros is None or self.__macros.declare is None:
            Convoy.exit_error(
                "To check if the declare macro is contained in the code, a declare macro must be specified."
            )

        return self.__macros.declare in self.__code

    def remove_comments(self) -> None:
        self.__code = re.sub(r"//.*", "", self.__code)
        self.__code = re.sub(r"/\*.*?\*/", "", self.__code, flags=re.DOTALL)

    def parse(
        self,
        *,
        line_delm: str = "\n",
        reserved_group_names: str | list[str] | None = None,
    ) -> list[Class]:

        if isinstance(reserved_group_names, str):
            reserved_group_names = [reserved_group_names]
        if reserved_group_names is None:
            reserved_group_names = []

        lines = self.__code.split(line_delm)
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
                found_macro = (
                    self.__macros is None or self.__macros.declare is None or self.__macros.declare in subline
                )
                found_end = subline.endswith("};")
                if found_macro or found_end:
                    break
            else:
                continue

            if found_end:
                continue

            template_line = (
                lines[i - 1]
                if i > 0
                and "template" in lines[i - 1]
                and "struct" not in lines[i - 1]
                and "class" not in lines[i - 1]
                else None
            )
            clstype = "class" if is_class else "struct"
            clinfo = self.__parse_class(sublines, template_line, clstype, namespaces, reserved_group_names)

            Convoy.verbose(f"Found and parsed <bold>{clinfo.name}</bold> {clstype}.")
            for field in clinfo.memfields.all:
                Convoy.verbose(f"  -Registered field <bold>{field.as_str(clinfo.name, is_static=False)}</bold>.")
            for field in clinfo.statfields.all:
                Convoy.verbose(f"  -Registered field <bold>{field.as_str(clinfo.name, is_static=True)}</bold>.")
            classes.append(clinfo)

        return classes

    def __parse_class(
        self,
        lines: list[str],
        template_line: str | None,
        clstype: str,
        namespaces: list[str],
        reserved_group_names: list[str],
        /,
    ) -> Class:
        clsline = lines[0].replace("template ", "template")
        name = re.match(rf".*{clstype} ([a-zA-Z0-9_<>, ]+)", clsline)
        if name is not None:
            name = name.group(1).strip()
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

        def check_group_macros(line: str, /) -> None:
            if self.__macros is None or self.__macros.group is None:
                return

            if self.__macros.group.begin in line:
                group = re.match(rf"{self.__macros.group.begin}\((.*?)\)", line)
                if group is not None:
                    group = group.group(1).replace('"', "")
                else:
                    Convoy.exit_error("Failed to match group name macro")
                properties = [p.strip() for p in group.split(",")]
                name = properties.pop(0)

                if name == "":
                    Convoy.exit_error("Group name cannot be empty.")
                if name in reserved_group_names:
                    Convoy.exit_error(
                        f"Group name cannot be <bold>{name}</bold>. It is listed as a reserved name: {reserved_group_names}"
                    )
                groups.append(Group(name, properties))

            elif self.__macros.group.end in line:
                groups.pop()

        def check_ignore_macros(line: str, /) -> bool:
            if self.__macros is None or self.__macros.ignore is None:
                return True

            nonlocal ignore
            if self.__macros.ignore.end in line:
                ignore = False
            elif ignore or self.__macros.ignore.begin in line:
                ignore = True
                return False

            return True

        for line in lines:
            line = line.strip()

            check_group_macros(line)
            if not check_ignore_macros(line):
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

            if not line.endswith(";") or "noexcept" in line or "override" in line:
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
                list({g.name: g for g in groups}.values()),
            )
            fields = static_fields if is_static else nstatic_fields
            fields_per_type = static_fields_per_type if is_static else nstatic_fields_per_type
            fields_per_group = static_fields_per_group if is_static else nstatic_fields_per_group

            fields.append(field)
            fields_per_type.setdefault(field.vtype, []).append(field)
            for group in groups:
                fields_per_group.setdefault(group, []).append(field)

        if self.__macros is not None:
            if ignore and self.__macros.ignore is not None:
                Convoy.exit_error(
                    f"Ignore macro was not closed properly with a <bold>{self.__macros.ignore.end}</bold>."
                )

            if groups and self.__macros.group is not None:
                Convoy.exit_error(
                    f"Group macro was not closed properly with a <bold>{self.__macros.group.end}</bold>."
                )

        return Class(
            name.strip(),
            namespaces,
            FieldCollection(nstatic_fields, nstatic_fields_per_type, nstatic_fields_per_group),
            FieldCollection(static_fields, static_fields_per_type, static_fields_per_group),
            template_decl,
        )
