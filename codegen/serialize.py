from cppgen import CPPFile
from argparse import ArgumentParser, Namespace
from pathlib import Path
from cpparser import ClassParser, ControlMacros, MacroPair, Class, Field

import sys

sys.path.append(str(Path(__file__).parent.parent))

from convoy import Convoy


def parse_arguments() -> Namespace:
    desc = """
    This python script takes in a C++ file and scans it for classes/structs marked with the
    toolkit macro TKIT_SERIALIZE_DECLARE. If it finds any instance of this macro, it will generate
    another C++ file containing a template specialization of a special serialization class (called Codec)
    which will contain the necessary code to serialize and deserialize the class members for the specified backend.

    It is also possible to group fields with the macros TKIT_SERIALIZE_GROUP_BEGIN and TKIT_SERIALIZE_GROUP_END,
    so that they may receive special treatment when serializing/deserializing through options passed as macro arguments
    such as: TKIT_SERIALIZE_GROUP_BEGIN("MyGroup", "--skip-if-missing", "serialize-as int").

    The list of options is the following:

    skip-if-missing: When deserializing, check if the field exists. If it doesnt, skip it silently.
    only-serialize: Create only serialization code for the selected fields.
    only-deserialize: Create only deserialization code for the selected fields.
    serialize-as <type>: Override the type of the selected fields and parse them with the specified one when serializing.
    deserialize-as <type>: Override the type of the selected fields and parse them with the specified one when deserializing.

    If some fields must be left out, the macros TKIT_SERIALIZE_IGNORE_BEGIN and TKIT_SERIALIZE_IGNORE_END can also
    be used.
    """
    parser = ArgumentParser(description=desc)

    parser.add_argument(
        "-i",
        "--input",
        type=Path,
        required=True,
        help="The C++ file to scan for the serialize macro.",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        required=True,
        help="The output file to write the serialization code to.",
    )
    parser.add_argument(
        "-b",
        "--backend",
        type=str,
        default="yaml",
        help="The serialization format to use. Currently, the only one supported is yaml.",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="Print more information.",
    )

    return parser.parse_args()


Convoy.log_label = "SERIALIZE"
args = parse_arguments()
Convoy.is_verbose = args.verbose

output: Path = args.output.resolve()
ffile: Path = args.input.resolve()
gpair = MacroPair(
    "TKIT_SERIALIZE_GROUP_BEGIN",
    "TKIT_SERIALIZE_GROUP_END",
)
ipair = MacroPair(
    "TKIT_SERIALIZE_IGNORE_BEGIN",
    "TKIT_SERIALIZE_IGNORE_END",
)
macros = ControlMacros(
    "TKIT_SERIALIZE_DECLARE",
    gpair,
    ipair,
)

with ffile.open("r") as f:
    content = f.read()
    parser = ClassParser(content, macros=macros)

    if not parser.has_declare_macro():
        Convoy.verbose(f"<fyellow>Macro '{macros.declare}' not found in file '{ffile}'. Exiting...")
        Convoy.exit_ok()
    classes = parser.parse()

if args.backend != "yaml":
    Convoy.exit_error(
        f"The serialization backend <bold>{args.backend}</bold> is not supported. Currently, only <bold>yaml</bold> is supported."
    )

options = ["skip-if-missing", "only-serialize", "only-deserialize", "serialize-as", "deserialize-as"]
hpp = CPPFile(output.name)
hpp.disclaimer("serialize.py")
hpp("#pragma once")
hpp.include(str(ffile.resolve()), quotes=True)
hpp.include(f"tkit/serialization/{args.backend}/codec.hpp", quotes=True)


def in_options(candidate: str, opts: list[str], /) -> bool:
    for op in opts:
        if op in candidate or candidate in op:
            return True
    return False


def ensure_options_consistency(options: list[str], /) -> None:
    if in_options("only-serialize", options) and in_options("only-deserialize", options):
        Convoy.exit_error(
            "Cannot have <bold>only-serialize</bold> and <bold>only-deserialize</bold> options at the same time."
        )


def get_fields_with_options(clsinfo: Class, /) -> list[tuple[Field, list[str]]]:
    result = []

    def gather_fields(fields: list[Field], /) -> None:
        for field in fields:
            opts = []
            for group in field.groups:
                for opt in group.properties:
                    if not in_options(opt, options):
                        Convoy.exit_error(
                            f"Unrecognized serialization option for group <bold>{group}</bold>: <bold>{opt}</bold>."
                        )
                    if opt not in opts:
                        opts.append(opt)

            ensure_options_consistency(opts)
            result.append((field, opts))

    gather_fields(clsinfo.memfields.all)
    gather_fields(clsinfo.statfields.all)

    return result


with hpp.scope(f"namespace TKit::{args.backend.capitalize()}", indent=0):
    for clsinfo in classes:
        fields = get_fields_with_options(clsinfo)

        for namespace in clsinfo.namespaces:
            if namespace != "TKit":
                hpp(f"using namespace {namespace}")

        with hpp.doc():
            hpp.brief(
                f"This is an auto-generated specialization of the placeholder `TKit::Codec` struct containing {args.backend} serialization code for `{clsinfo.name}`."
            )
            hpp(
                f"For serialization to work, this file must be included before any `TKit::Codec` instantiations occur. If `{clsinfo.name}` also includes fields that have automatic serialization code, such files must also be included."
            )
            hpp(
                f"You may customize how each field gets (de)serialized by grouping them with the macro pair `{gpair.begin}` and `{gpair.end}`, and adding options to the group to modify the generated code for the (de)serialization of those fields. The available options are the following:"
            )
            hpp(
                "- skip-if-missing: In deserialization, if the field is not found, skip it instead of raising a runtime error."
            )
            hpp("- only-serialize: Only serialize selected fields.")
            hpp("- only-deserialize: Only deserialize selected fields.")
            hpp("- serialize-as <type>: Override the field's type and use the provided one when serializing.")
            hpp("- deserialize-as <type>: Override the field's type and use the provided one when deserializing.")
            hpp(
                f'You may specify the above options with the group begin macro: `{gpair.begin}("GroupName", "--skip-if-missing", "serialize-as int")`.'
            )

        with hpp.scope(
            f"template <{clsinfo.template_decl if clsinfo.template_decl is not None else ''}> struct Codec<{clsinfo.name}>",
            closer="};",
        ):
            with hpp.doc():
                hpp.brief(f"Encode an instance of type `{clsinfo.name}` into a `Node` (serialization step).")
                hpp.param("p_Instance", f"An instance of type `{clsinfo.name}`.")
                hpp.ret("A node with serialization information.")

            with hpp.scope(f"static Node Encode(const {clsinfo.name} &p_Instance) noexcept"):
                hpp("Node node;")
                for field, options in fields:
                    if in_options("only-deserialize", options):
                        hpp.comment(f"Skipping {field.name} - It has been set as deserialization only")
                        continue

                    vtype = None
                    for opt in options:
                        if "serialize-as" in opt:
                            try:
                                vtype = opt.split(" ", 1)[1]
                                hpp.comment(f"The field type of `{field.name}` has been overriden by `{vtype}`.")
                                break
                            except IndexError:
                                Convoy.exit_error(
                                    f"Failed to parse option: <bold>serialize-as</bold>. Expected type name, but received: <bold>{opt}</bold>. Usage example: <bold>--serialize-as MyDesiredTypeName</bold>"
                                )

                    if vtype is None:
                        hpp(f'node["{field.name}"] = p_Instance.{field.name};')
                    else:
                        hpp(f'node["{field.name}"] = static_cast<{vtype}>(p_Instance.{field.name});')
                hpp("return node;")

            with hpp.doc():
                hpp.brief(f"Decode an instance of type `{clsinfo.name}` from a `Node` (deserialization step).")
                hpp.param("p_Node", "A node with serialization information.")
                hpp.param("p_Instance", f"An instance of type `{clsinfo.name}`.")

            with hpp.scope(f"static bool Decode(const Node &p_Node, {clsinfo.name} &p_Instance) noexcept"):
                with hpp.scope("if (!p_Node.IsMap())", delimiters=False):
                    hpp("return false;")

                for field, options in fields:
                    if in_options("only-serialize", options):
                        hpp.comment(f"Skipping `{field.name}` - It has been set as serialization only")
                        continue

                    vtype = field.vtype
                    for opt in options:
                        if "deserialize-as" in opt:
                            try:
                                vtype = opt.split(" ", 1)[1]
                                hpp.comment(f"The field type of `{field.name}` has been overriden by `{vtype}`.")
                                break
                            except IndexError:
                                Convoy.exit_error(
                                    f"Failed to parse option: <bold>deserialize-as</bold>. Expected type name, but received: <bold>{opt}</bold>. Usage example: <bold>--deserialize-as MyDesiredTypeName</bold>"
                                )

                    if in_options("skip-if-missing", options):
                        with hpp.scope(f'if (node["{field.name}"])', delimiters=False):
                            hpp(f'p_Instance.{field.name} = node["{field.name}"].as<{vtype}>();')
                    else:
                        hpp(f'p_Instance.{field.name} = node["{field.name}"].as<{vtype}>();')

                hpp("return true;")

hpp.write(output.parent)

Convoy.exit_ok()
