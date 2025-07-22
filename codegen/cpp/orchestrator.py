from pathlib import Path
from argparse import ArgumentParser, Namespace
from generator import CPPGenerator
from parser import CPParser, ClassCollection, ControlMacros, Class, Enum
from collections.abc import Callable

import sys

sys.path.append(str(Path(__file__).parent.parent.parent))

from convoy import Convoy


class CPPOrchestrator:

    def __init__(
        self,
        rinputs: str | Path | list[str | Path],
        routputs: str | Path | list[str | Path],
        /,
        *parse_args,
        macros: ControlMacros,
        recursive: bool = False,
        file_per_class: bool = False,
        **parse_kwargs,
    ) -> None:

        self.__file_per_class = file_per_class
        inputs = Convoy.resolve_paths(
            rinputs, recursive=recursive, check_exists=True, require_files=True, remove_duplicates=True
        )
        outputs = Convoy.resolve_paths(routputs, recursive=recursive, remove_duplicates=True)

        def log_paths(name: str, paths: list[Path], /) -> None:
            Convoy.verbose(f"Scanner resolved {len(paths)} {name} path(s).")
            for p in paths:
                Convoy.verbose(f" - <underline>{p}</underline>")

        log_paths("input", inputs)
        log_paths("output", outputs)

        parser = CPPOrchestrator.__create_parser(inputs, macros=macros)
        parser.remove_comments()

        classes = parser.parse(*parse_args, **parse_kwargs)

        self.__outputs = self.__validate_and_resolve_outputs(inputs, outputs, classes)
        self.__generators = self.__create_generators()

    def generate(
        self, generator: Callable[[CPPGenerator, ClassCollection], None], /, *, disclaimer: str | None = None
    ) -> None:
        for out, classes in self.__outputs.items():
            if not classes.classes and not classes.enums:
                continue
            gen = self.__generators[out]

            if disclaimer is not None:
                gen.disclaimer(disclaimer)
            gen("#pragma once")

            for c in classes.classes:
                gen.include(str(c.file), quotes=True)

            generator(gen, classes)

            gen.write(out)

    @classmethod
    def from_cli_arguments(cls, args: Namespace, /, *parser_args, macros: ControlMacros, **parser_kwargs):
        return cls(
            args.input,
            args.output,
            *parser_args,
            macros=macros,
            recursive=args.recursive,
            file_per_class=args.file_per_class,
            resolve_hierarchies_with_inheritance=args.use_inheritance_list,
            **parser_kwargs,
        )

    @classmethod
    def setup_cli_arguments(cls, parser: ArgumentParser, /, *, add_verbose: bool = False) -> None:
        parser.add_argument(
            "-i",
            "--input",
            type=Path,
            nargs="+",
            required=True,
            help="A path, list of path or pattern(s) for which all files contained will be scanned.",
        )
        parser.add_argument(
            "-o",
            "--output",
            type=Path,
            nargs="+",
            required=True,
            help="A path, list of paths or pattern(s) for which the generated code will be emitted. If the path is a folder, a file for each of the input files will be generated inside the specified folder, unless --file-per-class was specified. In that case, a file per scanned class or struct will be generated. If a pattern or list of paths is passed, it must be the same length as the number of inputs, and one file per input file will be emitted. If a single file path is passed, all emitted code will be written to that file.",
        )
        parser.add_argument(
            "-r",
            "--recursive",
            action="store_true",
            default=False,
            help="Whether to recursively search for input files in folders.",
        )
        parser.add_argument(
            "-f",
            "--file-per-class",
            action="store_true",
            default=False,
            help="Whether to generate a file per parsed class or struct. If selected, the output must be a single folder.",
        )
        parser.add_argument(
            "--use-inheritance-list",
            action="store_true",
            default=False,
            help="Use the class or struct inheritance list to resolve parents instead of the manually specified through the declare macros.",
        )
        if add_verbose:
            parser.add_argument(
                "-v",
                "--verbose",
                action="store_true",
                default=False,
                help="Print more information.",
            )

    def __create_generators(self) -> dict[Path, CPPGenerator]:
        return {out: CPPGenerator() for out in self.__outputs}

    def __validate_and_resolve_outputs(
        self, inputs: list[Path], outputs: list[Path], classes: ClassCollection, /
    ) -> dict[Path, ClassCollection]:
        if not inputs or not outputs:
            Convoy.exit_error("Must at least provide an input and an output.")

        if len(outputs) == 1 and outputs[0].is_dir():
            odir = outputs[0]
            if self.__file_per_class:
                return CPPOrchestrator.__resolve_outputs_with_classes(classes, odir)

            return CPPOrchestrator.__resolve_outputs_with_inputs(classes, odir)

        if self.__file_per_class:
            Convoy.exit_error(
                "If the option <bold>--file-per-class</bold> is selected, the output must be a single directory"
            )

        if len(outputs) != 1:
            if len(inputs) != len(outputs):
                Convoy.exit_error("If multiple output paths are provided, they must match the amount of input files.")

            inp_to_out = {inp: out for inp, out in zip(inputs, outputs)}
            result: dict[Path, ClassCollection] = {}

            def resolve(objs: list[Class] | list[Enum], /) -> None:
                for obj in objs:
                    if obj.file is None:
                        Convoy.exit_error(f"The class <bold>{obj.id.identifier}</bold> has no file attribute.")
                    out = inp_to_out[obj.file]
                    result.setdefault(out, ClassCollection()).add(obj)

            resolve(classes.classes)
            resolve(classes.enums)

            return result

        return {outputs[0]: classes}

    @classmethod
    def __resolve_outputs_with_classes(
        cls, classes: ClassCollection, directory: Path, /
    ) -> dict[Path, ClassCollection]:
        outputs = {}
        forbidden = r"<>:\"/\|?*"

        def check(name: str, /) -> str:
            if any(f in name for f in forbidden):
                Convoy.exit_error(
                    f"The name <bold>{name}</bold> contains forbidden characters that cannot be used as a file name. To avoid this error, do not set the <bold>--file-per-class</bold> option and choose another way to export the generated code."
                )
            return f"{Convoy.to_snake_case(name)}.hpp"

        for name, cs in classes.per_name.items():
            name = check(name)
            cc = ClassCollection()
            for c in cs:
                cc.add(c)
            outputs[directory / name] = cc
        for enum in classes.enums:
            name = check(enum.id.name)
            cc = ClassCollection()
            cc.add(enum)
            outputs[directory / name] = cc

        return outputs

    @classmethod
    def __resolve_outputs_with_inputs(
        cls, classes: ClassCollection, directory: Path, /
    ) -> dict[Path, ClassCollection]:
        outputs: dict[Path, ClassCollection] = {}

        def resolve(objs: list[Class] | list[Enum], /) -> None:
            for obj in objs:
                if obj.file is None:
                    Convoy.exit_error(f"The {obj.id.ctype} <bold>{obj.id.identifier}</bold> has no file attribute.")
                outputs.setdefault(directory / obj.file.name, ClassCollection()).add(obj)

        resolve(classes.classes)
        resolve(classes.enums)

        return outputs

    @classmethod
    def __create_parser(cls, inputs: list[Path], /, *, macros: ControlMacros) -> CPParser:
        code = {}
        for inp in inputs:
            with open(inp, "r") as f:
                code[inp] = f.read()

        return CPParser(code, macros=macros)
