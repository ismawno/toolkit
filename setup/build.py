from pathlib import Path
from configparser import ConfigParser
from argparse import ArgumentParser, Namespace

import os
import subprocess
import shutil

# Next steps: Add support for more generators


def create_default_settings_file(path: Path, /) -> ConfigParser:
    cfg = ConfigParser(allow_no_value=True)
    cfg.add_section("some-comments")
    cfg.set(
        "some-comments",
        "; This is an auto generated file. Edit it as you please, or create one on your own and specify it through the --settings-path argument with the build.py script",
    )
    cfg.set(
        "some-comments",
        "; The parameters provided through arguments in the build.py script will override whatever is in the settings file",
    )
    cfg.set(
        "some-comments",
        "; All path parameters must be relative to the root repository",
    )

    cfg.add_section("general")
    cfg.set("general", "; The build folder name should contain the word 'build'")
    cfg.set(
        "general",
        "build-path",
        "./build",
    )

    cfg.set("general", "; Debug, Release or Dist")
    cfg.set(
        "general",
        "build-type",
        "Debug",
    )

    cfg.set(
        "general",
        "; The current build folder will be deleted and rebuilt from scratch if this is set to 'ON'",
    )
    cfg.set(
        "general",
        "rebuild",
        "OFF",
    )

    cfg["targets"] = {"build-tests": "ON", "build-performance": "ON"}
    cfg["logging"] = {
        "info-logs": "ON",
        "warning-logs": "ON",
        "asserts": "ON",
        "silent-asserts": "OFF",
        "exceptions": "OFF",
        "log-colors": "ON",
    }
    cfg["memory"] = {"enable-block-allocator": "ON"}

    with open(path, "w") as f:
        cfg.write(f)
    return cfg


def load_settings_file(path: Path, /) -> ConfigParser:
    cfg = ConfigParser()
    cfg.read(path)
    return cfg


def create_arguments() -> tuple[Namespace, list[str]]:
    parser = ArgumentParser(
        description="This script attempts to ease out build process and have better control over CMake build settings. It is not intended to be used when toolkit is being built as a library in another project, but rather to build its executables, such as the testing or performance modules. The script expects a settings file to be provided. If it is not, and a default 'build-settings.ini' file is not found, it will create one with default parameters. If the provided settings path does not exist, it will do the same but with the provided path. All of the parameters provided through arguments will override whatever is in the settings file that was provided or automatically generated. All path parameters must be relative to the root repository",
    )

    # Require a value
    parser.add_argument(
        "-sp",
        "--settings-path",
        type=str,
        default=None,
        help="Path to the build settings file",
    )
    parser.add_argument(
        "-bp",
        "--build-path",
        type=str,
        default=None,
        help="Path to the build directory",
    )
    parser.add_argument(
        "-bt",
        "--build-type",
        type=str,
        default=None,
        help="Build type. Can be Debug, Release or Dist",
    )

    # Booleans
    parser.add_argument(
        "-r",
        "--rebuild",
        type=str,
        default=None,
        help="(ON or OFF) Whether to rebuild the project",
    )
    parser.add_argument(
        "-t",
        "--build-tests",
        type=str,
        default=None,
        help="(ON or OFF) Whether to build tests",
    )
    parser.add_argument(
        "-p",
        "--build-performance",
        type=str,
        default=None,
        help="(ON or OFF) Whether to build performance",
    )
    parser.add_argument(
        "-i",
        "--info-logs",
        type=str,
        default=None,
        help="(ON or OFF) Whether to log info messages",
    )
    parser.add_argument(
        "-w",
        "--warning-logs",
        type=str,
        default=None,
        help="(ON or OFF) Whether to log warning messages",
    )
    parser.add_argument(
        "-a",
        "--asserts",
        type=str,
        default=None,
        help="(ON or OFF) Whether to enable asserts",
    )
    parser.add_argument(
        "-s",
        "--silent-asserts",
        type=str,
        default=None,
        help="(ON or OFF) Whether to make asserts silent",
    )
    parser.add_argument(
        "-e",
        "--exceptions",
        type=str,
        default=None,
        help="(ON or OFF) Whether to enable exceptions",
    )
    parser.add_argument(
        "-c",
        "--log-colors",
        type=str,
        default=None,
        help="(ON or OFF) Whether to enable log colors",
    )
    parser.add_argument(
        "--enable-block-allocator",
        type=str,
        default=None,
        help="(ON or OFF) Whether to disable block allocator",
    )

    return parser.parse_known_args()


def create_cmake_parameters_map() -> dict[str, str]:
    return {
        "build-type": "CMAKE_BUILD_TYPE",
        "build-tests": "TOOLKIT_BUILD_TESTS",
        "build-performance": "TOOLKIT_BUILD_PERFORMANCE",
        "info-logs": "TOOLKIT_ENABLE_INFO_LOGS",
        "warning-logs": "TOOLKIT_ENABLE_WARNING_LOGS",
        "asserts": "TOOLKIT_ENABLE_ASSERTS",
        "silent-asserts": "TOOLKIT_SILENT_ASSERTS",
        "exceptions": "TOOLKIT_ENABLE_EXCEPTIONS",
        "log-colors": "TOOLKIT_ENABLE_LOG_COLORS",
        "enable-block-allocator": "TOOLKIT_ENABLE_BLOCK_ALLOCATOR",
    }


def main() -> None:
    args, build_args = create_arguments()

    settings_path = (
        Path(__file__).parent / "build-settings.ini"
        if args.settings_path is None
        else Path(args.settings_path)
    )
    if not settings_path.exists():
        print(
            f"The path '{settings_path}' does not exist. Creating a default settings file at that same location..."
        )
        cfg = create_default_settings_file(settings_path)
    else:
        cfg = load_settings_file(settings_path)

    build_settings: dict[str, str] = {}
    for section in cfg.sections():
        for key in cfg[section]:
            if key.startswith(";"):
                continue
            value = getattr(args, key.replace("-", "_"))
            build_settings[key] = value if value is not None else cfg[section][key]

    cmake_map = create_cmake_parameters_map()
    cmake_settings = {v: build_settings[k] for k, v in cmake_map.items()}
    cmake_args = [f"-D{key}={value}" for key, value in cmake_settings.items()]

    root = Path(__file__).parent.parent
    build_folder = root / build_settings["build-path"]
    if build_settings["rebuild"] == "ON":
        shutil.rmtree(build_folder, ignore_errors=True)
    if not build_folder.exists():
        build_folder.mkdir(parents=True, exist_ok=True)

    os.chdir(root)
    subprocess.run(["cmake", "-B", build_folder, "-S", "."] + cmake_args, check=True)
    subprocess.run(
        ["cmake", "--build", build_folder, "--config", build_settings["build-type"]]
        + build_args,
        check=True,
    )


if __name__ == "__main__":
    main()
