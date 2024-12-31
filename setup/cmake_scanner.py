from pathlib import Path
from argparse import ArgumentParser, Namespace
from configparser import ConfigParser


def parse_arguments() -> Namespace:
    desc = """
    This is a CMake scanner whose sole purpose is to avoid the evil thing that is CMake's cache.
    When running a CMake script, I would like to know what the script is doing, and what it is going to do.
    This script will scan the CMakeLists.txt file and detect configuration options, so that it can
    parse them and specify them explicitly to nullify the effects of the cache, so that I have no surprises.
    
    This script will create a build.ini file in the same directory this file lives, to be used by build.py to do
    the actual building.
    """

    parser = ArgumentParser(description=desc)
    parser.add_argument(
        "-p",
        "--path",
        type=Path,
        required=True,
        help="The path to search for CMakelists.txt",
    )
    parser.add_argument(
        "--hint",
        default="define_option",
        type=str,
        help="A hint to let the scanner now how to look for options",
    )
    parser.add_argument(
        "--strip-preffix",
        action="store_true",
        default=True,
        help="Strip the preffix from the option name, such as CMAKE_ or PROJECT_",
    )
    parser.add_argument(
        "--preffixes",
        action="append",
        # Make sure largest preffixes are first
        default=[
            "CMAKE_",
            "TOOLKIT_ENABLE_",
            "VULKIT_ENABLE_",
            "ONYX_ENABLE_",
            "TOOLKIT_",
            "VULKIT_",
            "ONYX_",
        ],
        help="The preffixes to strip.",
    )
    parser.add_argument(
        "--lower-case",
        action="store_true",
        default=True,
        help="Lower case the option names",
    )
    parser.add_argument(
        "--dyphen-separator",
        action="store_true",
        default=True,
        help="Use dyphen separator for option names",
    )

    return parser.parse_args()


def main() -> None:
    args = parse_arguments()
    hint: str = args.hint
    cmake_path: Path = args.path.resolve()
    strip_preffix: bool = args.strip_preffix
    preffixes: list[str] = args.preffixes
    lower_case: bool = args.lower_case
    dyphen_separator: bool = args.dyphen_separator
    cnd_logs: bool = args.enable_toolkit_conditional_logs

    def process_option(content: list[str]) -> tuple[str, str, str | bool]:
        ogvarname, val = content
        print(f"Detected option '{ogvarname}' with default value '{val}'")

        varname = ogvarname
        if strip_preffix:
            for preffix in preffixes:
                if varname.startswith(preffix):
                    varname = varname.removeprefix(preffix)
        if lower_case:
            varname = varname.lower()
        if dyphen_separator:
            varname = varname.replace("_", "-")

        if val == "ON":
            val = True
        elif val == "OFF":
            val = False

        return ogvarname, varname, val

    contents = []
    for cmake_file in cmake_path.rglob("CMakeLists.txt"):
        print(f"Scanning file at '{cmake_file}'")
        with open(cmake_file, "r") as f:
            contents.extend(
                [
                    process_option(
                        content.removeprefix(f"{hint}(").removesuffix(")").split(" ")
                    )
                    for content in f.read().splitlines()
                    if content.startswith(hint)
                ]
            )

    cfg = ConfigParser()
    cfg.add_section("default-values")
    for ogvarname, varname, val in contents:
        cfg["default-values"][ogvarname] = f"{varname}: {val}"

    path = Path(__file__).parent
    with open(path / "build.ini", "w") as f:
        cfg.write(f)

    option_value_pairs = {
        ogvarname: (varname, val) for ogvarname, varname, val in contents
    }

    def enable_cnd_log(ogvarname: str) -> None:
        btype = option_value_pairs["CMAKE_BUILD_TYPE"][1]
        varname = option_value_pairs[ogvarname][0]
        option_value_pairs[ogvarname] = (varname, btype != "Dist")

    if cnd_logs and "CMAKE_BUILD_TYPE" in option_value_pairs:
        enable_cnd_log("TOOLKIT_ENABLE_INFO_LOGS")
        enable_cnd_log("TOOLKIT_ENABLE_WARNING_LOGS")
        enable_cnd_log("TOOLKIT_ENABLE_LOG_COLORS")
        enable_cnd_log("TOOLKIT_ENABLE_ASSERTS")

    print(option_value_pairs)


if __name__ == "__main__":
    main()
