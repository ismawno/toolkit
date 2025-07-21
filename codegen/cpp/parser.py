from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path

import sys
import re

sys.path.append(str(Path(__file__).parent.parent.parent))

from convoy import Convoy


@dataclass(frozen=True)
class MacroPair:
    begin: str
    end: str


@dataclass(frozen=True)
class ControlMacros:
    declare: str
    group: MacroPair
    ignore: MacroPair


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
    fields: list[Field] = field(default_factory=list)
    per_name: dict[str, Field] = field(default_factory=dict)
    per_type: dict[str, list[Field]] = field(default_factory=dict)
    per_group: dict[Group, list[Field]] = field(default_factory=dict)

    def add(self, f: Field, /) -> None:
        if f.name in self.per_name:
            Convoy.exit_error(f"Tried to add a field that already exists: <bold>{f.name}</bold>.")
        self.fields.append(f)
        self.per_name[f.name] = f
        self.per_type.setdefault(f.vtype, []).append(f)
        for g in f.groups:
            self.per_group.setdefault(g, []).append(f)


@dataclass(frozen=True)
class ClassIdentifier:
    identifier: str
    name: str
    ctype: str
    templdecl: str | None
    inheritance: list[str]

    def template_matches(self, templargs: str, /) -> int:
        if self.templdecl is None:
            Convoy.exit_error(f"Cannot measure template matches in a non template {self.ctype}.")

        templdecl = CPParser._split_template_list(self.templdecl)
        templvars1, templvars2 = self.template_arguments_instantiation_pairs(templargs)
        matches = 0
        for t1, t2 in zip(templvars1, templvars2):
            if t1 == t2:
                matches += 1
            elif t1 not in templdecl:
                return -1

        return matches

    def template_arguments_instantiation_pairs(self, templargs: str, /) -> tuple[list[str], list[str]]:
        templvars1 = self.identifier.split("<", 1)[1].strip(">").strip().replace(", ", ",").split(",")
        templvars2 = templargs.replace(", ", ",").split(",")
        if len(templvars1) != len(templvars2):
            Convoy.exit_error(
                f"The amount of template arguments between the class declaration to instantiate (<bold>{self.id.identifier}</bold>) and the ones to instantiate (<bold>{self.id.name}<{templargs}></bold>) does not match."
            )
        return templvars1, templvars2


@dataclass(frozen=True)
class Class:
    id: ClassIdentifier
    parents: list[Class]
    namespaces: list[str]
    member: FieldCollection
    static: FieldCollection
    file: Path | None

    def instantiate(self, templargs: str, /) -> Class:
        Convoy.log(f"Instantiating <bold>{self.id.identifier}</bold> to <bold>{self.id.name}<{templargs}></bold>.")
        if self.id.templdecl is None:
            Convoy.exit_error(f"Cannot instantiate a non-template {self.id.ctype}.")

        templvars1, templvars2 = self.id.template_arguments_instantiation_pairs(templargs)
        templdecl = CPParser._split_template_list(self.id.templdecl)

        member = FieldCollection()
        static = FieldCollection()
        tdecl: str | list[str] = []
        for t1, t2 in zip(templvars1, templvars2):
            if t1 == t2:
                if t1 in templdecl:
                    idx = templdecl.index(t1)
                    tdecl.append(self.id.templdecl.replace(", ", ",").split(",")[idx])

                continue
            if t1 not in templdecl:
                Convoy.exit_error(
                    f"The template argument to instantiate, <bold>{t1}</bold>, is not present in the template declaration list: <bold>{self.id.templdecl}</bold>."
                )

            Convoy.verbose(
                f"Instantiating <bold>{t1}</bold> to <bold>{t2}</bold> in all fields of <bold>{self.id.identifier}</bold>."
            )

            def apply(fcol: FieldCollection, outcol: FieldCollection, /) -> None:
                for f in fcol.fields:
                    vtype = re.sub(rf"\b{t1}\b", t2, f.vtype)
                    fi = Field(f.name, f.visibility, vtype, f.groups)
                    outcol.add(fi)

            apply(self.member, member)
            apply(self.static, static)

        tdecl = ", ".join(tdecl)
        idf = f"{self.id.name}<{templargs}>"
        Convoy.verbose(f"Successfully instantiated <bold>{self.id.identifier}</bold> to <bold>{idf}</bold>.")
        if tdecl:
            Convoy.verbose(
                f"The template declaration went from <bold>template <{self.id.templdecl}></bold> to <bold>template <{tdecl}></bold>."
            )
        else:
            Convoy.verbose(
                f"The template declaration (<bold>template <{self.id.templdecl}></bold>) was fully instantiated."
            )

        id = ClassIdentifier(idf, self.id.name, self.id.ctype, tdecl if tdecl else None, self.id.inheritance)
        return Class(id, self.parents, self.namespaces, member, static, self.file)


@dataclass(frozen=True)
class ClassCollection:
    classes: list[Class] = field(default_factory=list)
    per_name: dict[str, list[Class]] = field(default_factory=dict)
    per_identifier: dict[str, Class] = field(default_factory=dict)

    def add(self, c: Class, /) -> None:
        if c.id.identifier in self.per_identifier:
            Convoy.exit_error(f"Tried to add a class that already exists: <bold>{c.id.identifier}</bold>.")
        self.classes.append(c)
        self.per_name.setdefault(c.id.name, []).append(c)
        self.per_identifier[c.id.identifier] = c


@dataclass(frozen=True)
class _ClassInfo:
    id: ClassIdentifier
    file: Path | None
    namespaces: list[str]
    body: list[str]
    macro_args: list[str]
    has_declare_macro: bool


@dataclass(frozen=True)
class _ClassInfoCollection:
    classes: list[_ClassInfo] = field(default_factory=list)
    per_name: dict[str, list[_ClassInfo]] = field(default_factory=dict)
    per_identifier: dict[str, _ClassInfo] = field(default_factory=dict)

    def add(self, c: _ClassInfo, /) -> None:
        if c.id.identifier in self.per_identifier:
            Convoy.exit_error(f"Tried to add a class info that already exists: <bold>{c.id.identifier}</bold>.")
        self.classes.append(c)
        self.per_name.setdefault(c.id.name, []).append(c)
        self.per_identifier[c.id.identifier] = c


class CPParser:

    def __init__(self, code: str | dict[Path, str], /, *, macros: ControlMacros) -> None:
        if not isinstance(code, str):
            code = CPParser.__merge_code(code)
        self.__code = (
            code.replace("<class", "<typename").replace(",class", ",typename").replace("template ", "template")
        )
        self.__macros = macros
        self.__cache = ClassCollection()

    def has_declare_macro(self) -> bool:
        return self.__macros.declare in self.__code

    def clear_cache(self) -> None:
        self.__cache = ClassCollection()

    def remove_comments(self) -> None:
        self.__code = re.sub(r"//(?!\s*CPParser file:).*", "", self.__code)
        self.__code = re.sub(r"/\*.*?\*/", "", self.__code, flags=re.DOTALL)

    def parse(
        self,
        *,
        line_delm: str = "\n",
        reserved_group_names: str | list[str] | None = None,
        resolve_hierarchies_with_inheritance: bool = False,
    ) -> ClassCollection:

        if isinstance(reserved_group_names, str):
            reserved_group_names = [reserved_group_names]
        if reserved_group_names is None:
            reserved_group_names = []

        classes = ClassCollection()
        clinfos = self.__find_classes(line_delm=line_delm)

        def parse_class(clinfo: _ClassInfo, /, *, override_declare_macro=False) -> Class | None:
            declm = self.__macros.declare
            if clinfo.has_declare_macro:
                pids = clinfo.macro_args.copy()
                if not pids:
                    Convoy.exit_error(
                        f"The first argument of the declare macro <bold>{declm}</bold> must be the {clinfo.id.ctype} name, but it has currently no arguments. Expected <bold>{clinfo.id.name}</bold>."
                    )
                name = pids.pop(0)
                if name != clinfo.id.name:
                    Convoy.exit_error(
                        f"The first argument of the declare macro <bold>{declm}</bold> must be the {clinfo.id.ctype} name, but it found <bold>{name}</bold>. Expected: <bold>{clinfo.id.name}</bold>."
                    )
                if resolve_hierarchies_with_inheritance:
                    pids = clinfo.id.inheritance
            elif override_declare_macro:
                pids = clinfo.id.inheritance if resolve_hierarchies_with_inheritance else []
            else:
                return None

            parents = []
            for pid in pids:
                if pid in self.__cache.per_identifier:
                    parents.append(self.__cache.per_identifier[pid])
                    continue
                needs_instantiation = pid not in clinfos.per_identifier
                templargs = pid.split("<", 1)[1].strip(">").strip()
                if needs_instantiation:
                    Convoy.log(
                        f"The parent identifier <bold>{pid}</bold> of the {clinfo.id.ctype} <bold>{clinfo.id.identifier}</bold> was not found explicitly. Attempting to instantiate from a general definition..."
                    )
                    errfn = Convoy.warning if resolve_hierarchies_with_inheritance else Convoy.exit_error

                    if "<" not in pid:
                        errfn(
                            f"The parent identifier <bold>{pid}</bold> is not templated. An instantiation is not possible."
                        )
                        continue

                    pname = pid.split("<", 1)[0]
                    if pname not in clinfos.per_name:
                        errfn(
                            f"The parent name <bold>{pname}</bold>, extracted from the identifier <bold>{pid}</bold>, of the {clinfo.id.ctype} <bold>{clinfo.id.identifier}</bold> was not found, and so it is not possible to instantiate and resolve the parent's definition."
                        )
                        continue
                    max_matches = 0
                    pclinfo = None
                    for p in clinfos.per_name[pname]:
                        if p.id.templdecl is None:
                            Convoy.verbose(
                                f"The {p.id.ctype} <bold>{p.id.identifier}</bold> is not elligible as it does not have a template declaration."
                            )
                            continue
                        matches = p.id.template_matches(templargs)
                        if matches >= max_matches:
                            pclinfo = p
                            max_matches = matches

                    if pclinfo is None:
                        errfn(f"No elligible class or struct was found from which to instantiate <bold>{pid}</bold>.")
                        continue
                    else:
                        Convoy.verbose(
                            f"Found an elligible {pclinfo.id.ctype} to instantiate: <bold>{pclinfo.id.identifier}</bold>."
                        )

                else:
                    pclinfo = clinfos.per_identifier[pid]

                c = parse_class(pclinfo, override_declare_macro=True)
                if c is None:
                    Convoy.exit_error(
                        f"The function parse_class returned None when overriding declare macro for parent <bold>{pid}</bold>. It should not happen."
                    )

                if needs_instantiation:
                    c = c.instantiate(templargs)
                parents.append(c)

            return self.__gather_fields(clinfo, parents, reserved_group_names)

        for clinfo in clinfos.classes:
            cl = parse_class(clinfo)
            if cl is not None:
                classes.add(cl)

        for c in classes.classes:
            for parent in c.parents:

                def add_fields(pfields: FieldCollection, cfields: FieldCollection, /, *, static: bool) -> None:
                    for f in pfields.fields:
                        if f.name in cfields.per_name:
                            Convoy.warning(
                                f" - The field <bold>{f.as_str(parent.id.identifier, is_static=static)}</bold> is being shadowed by a field with the same name in the child {c.ctype} <bold>{c.id.identifier}</bold>."
                            )
                        else:
                            Convoy.verbose(
                                f" - The field <bold>{f.as_str(parent.id.identifier, is_static=static)}</bold> has been inherited by <bold>{c.id.identifier}</bold>."
                            )
                            cfields.add(f)

                Convoy.log(
                    f"Inheriting fields from <bold>{c.id.identifier}</bold> parent: <bold>{parent.id.identifier}</bold>."
                )
                add_fields(parent.member, c.member, static=False)
                add_fields(parent.static, c.static, static=True)

        return classes

    def __find_classes(
        self,
        *,
        line_delm: str = "\n",
    ) -> _ClassInfoCollection:

        lines = self.__code.split(line_delm)
        namespaces = []
        file = None
        index = 0
        classes = _ClassInfoCollection()
        while index < len(lines):
            line = lines[index].strip()
            if "CPParser file" in line:
                file = Path(line.split(": ", 1)[1].strip("\n").strip())

            if line.endswith(";"):
                index += 1
                continue

            match = re.match(r"namespace ([a-zA-Z0-9_::]+)", line)
            if match is not None:
                namespace = match.group(1).split("::")
                namespaces.extend(namespace)
                index += 1
                continue

            is_class = "class" in line and not "enum" in line
            is_struct = not is_class and "struct" in line

            if not is_class and not is_struct:
                index += 1
                continue
            clstype = "class" if is_class else "struct"

            start = index
            end = len(lines)
            macro_args = []
            has_declm = False

            template_line = (
                lines[index - 1].strip()
                if index > 0
                and "template" in lines[index - 1]
                and "struct" not in lines[index - 1]
                and "class" not in lines[index - 1]
                else None
            )

            Convoy.verbose(f"Found a {clstype} declaration. ")
            if template_line is not None:
                if template_line.count("template") > 1:
                    Convoy.warning(f"Nested template arguments are not supported: <bold>{template_line}</bold>.")
                    index += 1
                    continue

                Convoy.verbose(f" - <bold>{template_line}</bold>")
            Convoy.verbose(f" - <bold>{lines[index].strip()}</bold>")

            while index < end:
                subline = lines[index].strip()
                declm = self.__macros.declare
                if declm in subline:
                    if has_declm:
                        Convoy.exit_error(f"Found a duplicate declare macro statement for the {clstype}.")
                    Convoy.log(f"Found a {clstype} marked with the declare macro <bold>{declm}</bold>.")

                    mtch = re.match(rf"{declm}\((.*?)\)", subline)
                    if mtch is None:
                        Convoy.exit_error(
                            f"Failed to match declare macro arguments for the line <bold>{subline}</bold>. Declare macro: <bold>{declm}</bold>."
                        )
                    macro_args = [m.strip() for m in Convoy.nested_split(mtch.group(1), ",", openers="<", closers=">")]
                    has_declm = True

                if subline == "};":
                    end = index + 1
                    break

                index += 1

            clsdecl = lines[start] if template_line is None else lines[start] + "\n" + template_line
            clsbody = lines[start + 1 : end]

            identifier = self.__parse_class_identifier(clsdecl, clstype)
            if identifier.identifier in classes.per_identifier:
                Convoy.exit_error(
                    f"Found a {clstype} with a duplicate identifier: <bold>{identifier.identifier}</bold>."
                )

            clinfo = _ClassInfo(identifier, file, namespaces, clsbody, macro_args, has_declm)
            classes.add(clinfo)

            index += 1
        return classes

    @classmethod
    def __merge_code(cls, code: dict[Path, str], /) -> str:
        merged = ""
        for p, c in code.items():
            merged += f"\n// CPParser file: {p}\n" + c
        return merged

    @classmethod
    def _split_template_list(cls, tlist: str, /) -> list[str]:
        return [
            Convoy.nested_split(var, " ", openers="<", closers=">", n=1)[1]
            for var in tlist.replace(", ", ",").split(",")
        ]

    def __parse_class_identifier(self, clsdecl: str, clstype: str, /) -> ClassIdentifier:
        Convoy.verbose(f"Attempting to parse {clstype} identifier.")

        pattern = r"(?:template<(.*)>[\n ]*)?(?:class|struct)(?: alignas\(.*?\))?(?: [a-zA-Z0-9_]+)? ([a-zA-Z0-9_<>,:]+) ?(?:: ?([a-zA-Z0-9_<>, :]+))?"
        declaration = re.match(pattern, clsdecl.replace(", ", ","))
        if declaration is None:
            Convoy.exit_error(
                f"A match was not found when trying to extract the name of the {clstype}. The identified declaration was the following: <bold>{clsdecl}</bold>."
            )
        templdecl = declaration.group(1)
        identifier = declaration.group(2)
        inheritance = declaration.group(3)
        if not templdecl:
            templdecl = None

        if identifier is not None:
            identifier = identifier.strip().replace(",", ", ")
            Convoy.verbose(f" - Extracted initial identifier: <bold>{identifier}</bold>.")
        else:
            Convoy.exit_error(
                f"Failed to extract a {clstype} identifier with the following declaration line: <bold>{clsdecl}</bold>."
            )
        if templdecl is not None:
            templdecl.strip()
            Convoy.verbose(f" - Extracted template arguments declaration: <bold>{templdecl}</bold>.")
        else:
            Convoy.verbose(" - No template declaration was found.")

        if inheritance is not None:
            inheritance.replace("public", "").replace("private", "").replace("protected", "").strip()
            Convoy.verbose(f" - Extracted inheritance list: <bold>{inheritance}</bold>.")
        else:
            Convoy.verbose(" - No inheritance list was found.")

        if templdecl is not None and "<" not in identifier:
            template_vars = ", ".join(CPParser._split_template_list(templdecl))

            identifier = f"{identifier}<{template_vars}>"
            Convoy.verbose(
                f" - Generated a more accurate identifier with template arguments: <bold>{identifier}</bold>."
            )

        name = identifier.split("<", 1)[0]
        return ClassIdentifier(
            identifier,
            name,
            clstype,
            templdecl,
            (
                [inh.strip() for inh in Convoy.nested_split(inheritance, ",", openers="<", closers=">")]
                if inheritance is not None
                else []
            ),
        )

    def __gather_fields(
        self,
        clinfo: _ClassInfo,
        parents: list[Class],
        reserved_group_names: list[str],
        /,
    ) -> Class:
        if clinfo.id.identifier in self.__cache.per_identifier:
            return self.__cache.per_identifier[clinfo.id.identifier]

        Convoy.log(f"Gathering fields for {clinfo.id.ctype} <bold>{clinfo.id.identifier}</bold>.")
        if not clinfo.has_declare_macro:
            Convoy.log(
                f"This {clinfo.id.ctype} was not explicitly marked with the declare macro, but it is being parsed because another class or struct is inheriting its fields."
            )

        groups = []

        static = FieldCollection()
        member = FieldCollection()

        scope_counter = 0
        ignore = False

        visibility = "private" if clinfo.id.ctype == "class" else "public"

        def check_group_macros(line: str, /) -> None:
            if self.__macros.group.begin in line:
                group = re.match(rf"{self.__macros.group.begin}\((.*?)\)", line)
                if group is not None:
                    group = group.group(1).replace('"', "")
                else:
                    Convoy.exit_error("Failed to match group name macro.")
                properties = group.replace(", ", ",").split(",")
                name = properties.pop(0)
                if name == "":
                    Convoy.exit_error("Group name cannot be empty.")
                if name in reserved_group_names:
                    Convoy.exit_error(
                        f"Group name cannot be <bold>{name}</bold>. It is listed as a reserved name: {reserved_group_names}."
                    )

                if not properties:
                    Convoy.verbose(f" - Pushing group <bold>{name}</bold>.")
                else:
                    Convoy.verbose(
                        f" - Pushing group <bold>{name}</bold> with properties <bold>{', '.join(properties)}</bold>."
                    )

                groups.append(Group(name, properties))

            elif self.__macros.group.end in line:
                name = groups.pop()
                Convoy.verbose(f" - Popping group <bold>{name}</bold>.")

        def check_ignore_macros(line: str, /) -> bool:
            nonlocal ignore
            if self.__macros.ignore.end in line:
                ignore = False
            elif ignore or self.__macros.ignore.begin in line:
                ignore = True
                return False

            return True

        for line in clinfo.body:
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

            if not line.endswith(";") or "noexcept" in line or "override" in line or "using" in line:
                continue

            line = line.replace(";", "").strip().removeprefix("inline ")
            if "=" not in line and "{" not in line and ("(" in line or ")" in line):
                continue

            is_static = line.startswith("static")
            line = line.removeprefix("static ").removeprefix("inline ")

            line = re.sub(r"=.*", "", line)
            line = re.sub(r"{.*", "", line).strip()

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

            Convoy.verbose(
                f" - Registered field <bold>{field.as_str(clinfo.id.identifier, is_static=is_static)}</bold>."
            )
            fields = static if is_static else member
            fields.add(field)

        if ignore:
            Convoy.exit_error(f"Ignore macro was not closed properly with a <bold>{self.__macros.ignore.end}</bold>.")

        if groups:
            Convoy.exit_error(f"Group macro was not closed properly with a <bold>{self.__macros.group.end}</bold>.")

        cl = Class(
            clinfo.id,
            parents,
            clinfo.namespaces,
            member,
            static,
            clinfo.file,
        )
        self.__cache.add(cl)
        return cl
