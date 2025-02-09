from pathlib import Path
from argparse import ArgumentParser, Namespace
from configparser import ConfigParser

import subprocess
import os


def try_convert_bool(val: str) -> bool | str:
    if val == "True":
        return True
    elif val == "False":
        return False
    return val


def load_build_ini() -> ConfigParser:
    cfg = ConfigParser()

    root = Path(__file__).parent
    path = root / "build.ini"
    if not path.exists():
        raise FileNotFoundError(f"No 'build.ini' file found in {root}")

    with open(path) as f:
        cfg.read_file(f)

    return cfg


def parse_section(cfg: ConfigParser, section: str) -> dict[str, tuple[str, str | bool]]:
    parsed = {}
    for ogvarname, kv in cfg[section].items():
        varname, val = kv.split(": ")
        if varname in parsed:
            raise ValueError(
                f"Name mismatch! Variable '{varname}' already exists in the parsed dictionary"
            )

        parsed[ogvarname] = (varname, try_convert_bool(val))

    return parsed


def parse_arguments(
    arguments: dict[str, tuple[str, str | bool]]
) -> tuple[Namespace, dict[str, str | bool]]:
    desc = """
    This script takes in a 'build.ini' configuration file created from 'cmake_scanner.py' and
    runs CMake with the options specified in the configuration file, avoiding cache shit.

    This file must be in the same directory as the 'build.ini' file to work.
    """

    parser = ArgumentParser(description=desc)
    parser.add_argument(
        "-b",
        "--build",
        default=Path("build"),
        type=Path,
        help="The location where the project will be built.",
    )
    parser.add_argument(
        "-s",
        "--source",
        default=Path("."),
        type=Path,
        help="The location where the source files are found (where the root CMakeLists.txt is).",
    )

    arg_map = {}
    for ogvarname, (varname, val) in arguments.items():
        if isinstance(val, bool):
            cmakeval = "ON" if val else "OFF"
            parser.add_argument(
                f"--{varname}",
                action="store_true",
                default=False,
                help=f"Ensures that the CMake option '{ogvarname.upper()}' is set of 'ON'. Its default is '{cmakeval}'.",
            )
            parser.add_argument(
                f"--no-{varname}",
                action="store_true",
                default=False,
                help=f"Ensures that the CMake option '{ogvarname.upper()}' is set of 'OFF'. Its default is '{cmakeval}'.",
            )
        else:
            parser.add_argument(
                f"--{varname}",
                type=str,
                default=val,
                help=f"Set the value of '{ogvarname.upper()}' to the specified value. Default is '{val}'.",
            )

        arg_map[varname.replace("-", "_")] = ogvarname, val
    return parser.parse_args(), arg_map


def main() -> None:
    cfg = load_build_ini()
    arguments = parse_section(cfg, "default-values")
    args, arg_map = parse_arguments(arguments)

    build_path: Path = args.build.resolve()
    source_path: Path = args.source.resolve()

    arg_dict = vars(args)
    for k, v in arg_dict.items():
        if k not in arg_map:
            continue

        ogvarname, default = arg_map[k]
        if not isinstance(v, bool):
            value = v
        else:
            nv = arg_dict[f"no_{k}"]
            if v and nv:
                raise ValueError(
                    f"Cannot set both '{k}' and 'no_{k}' to True. Please choose one"
                )
            if v:
                value = "True"
            elif nv:
                value = "False"
            else:
                value = "True" if default else "False"

        ogsection = f"{ogvarname.replace('_', '-').lower()}.{value}"
        if ogsection not in cfg:
            continue

        overrides = cfg[ogsection]
        for varname, new_default in overrides.items():
            varname = varname.replace("-", "_")
            if varname not in arg_map:
                raise ValueError(
                    f"Default value override '{varname}' was not found in the 'default-values' section"
                )
            new_value = (arg_map[varname][0], try_convert_bool(new_default))
            arg_map[varname] = new_value

    cmake_args = {}
    for k, v in arg_dict.items():
        if k not in arg_map:
            continue

        ogvarname, default = arg_map[k]
        if not isinstance(v, bool):
            cmake_args[f"-D{ogvarname.upper()}"] = v.replace('"', "")
            continue

        nv = arg_dict[f"no_{k}"]

        if v:
            cmake_args[f"-D{ogvarname.upper()}"] = "ON"
        elif nv:
            cmake_args[f"-D{ogvarname.upper()}"] = "OFF"
        else:
            cmake_args[f"-D{ogvarname.upper()}"] = "ON" if default else "OFF"

    cmake_args = [f"{k}={v}" for k, v in cmake_args.items()]
    build_path.mkdir(exist_ok=True, parents=True)

    print("Running CMake with the following arguments:")
    for arg in cmake_args:
        print(f"    {arg}")

    os.chdir(build_path)
    subprocess.run(["cmake", str(source_path)] + cmake_args, check=True)


if __name__ == "__main__":
    main()
