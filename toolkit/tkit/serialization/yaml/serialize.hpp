#pragma once

/**
 * These serialization macros are placeholder macros that will trigger serialization code generation scripts. These
 * triggers are set up with CMake, so the project must be built with it for it to work. Marking classes/structs with
 * these macros alone won't work either, as an extra step is needed: When listing the source files of your project,
 * after calling the CMake functions add_executable or add_library, you must call the CMake function
 * tkit_register_for_serialization, with the target and the source files the code generation scripts will have to scan
 * to find the marks.
 *
 * The generated code will have serialization and deserialization instructions for any struct or class appriopriately
 * marked compatible with the selected backend (currently only yaml is supported), and will live relative to this very
 * folder. This means generated serialization code should appear next to this file. The output of the serialization
 * system will follow your folder structure to ensure no mismatches/overwrites happen. Here is an example:
 *
 * Assuming the folder structure follows:
 * `${workspaceFolder}/project-name/project-small-identifier/my_marked_classes.hpp`
 *
 * then, assuming a typical CMake project setup, the output file will be located in the following path:
 * `${workspaceFolder}/build/_deps/toolkit-src/toolkit/tkit/serialization/project-small-identifier/my_marked_classes.hpp`
 * and you may include it in your code as follows:
 * `#include "tkit/serialization/yaml/project-small-identifier/my_marked_classes.hpp"`.
 */

/**
 * The main serialization macro, used to mark classes or structs required for serialization. Unmarked classes or structs
 * will be ignored. The macro expands to a friend statement so that the Codec class may have access to private fields.
 *
 * The extra arguments can be used to list the parents of the target class so it can also inherit its fields.
 */
#ifdef TKIT_ENABLE_YAML_SERIALIZATION
#    include "tkit/serialization/yaml/codec.hpp"
#    define TKIT_YAML_SERIALIZE_DECLARE(p_ClassName, ...) friend struct TKit::Yaml::Codec<p_ClassName>;
#else
#    define TKIT_YAML_SERIALIZE_DECLARE(p_ClassName, ...)
#endif

/**
 * A pair of macros that will allow you to customize how each field gets (de)serialized. The available options are the
 * following:
 *
 * - `skip-if-missing`: In deserialization, if the field is not found, skip it instead of raising a runtime error.
 *
 * - `only-serialize`: Only serialize selected fields.
 *
 * - `only-deserialize`: Only deserialize selected fields.
 *
 * - `serialize-as` <type>: Override the field's type and use the provided one when serializing.
 *
 * - `deserialize-as` <type>: Override the field's type and use the provided one when deserializing.
 *
 * You may specify the above options with the group begin macro: `TKIT_YAML_SERIALIZE_GROUP_BEGIN("GroupName",
 * "--skip-if-missing", "serialize-as int")`.
 */
#define TKIT_YAML_SERIALIZE_GROUP_BEGIN(...)
#define TKIT_YAML_SERIALIZE_GROUP_END()

/**
 * This pair of macros will allow you to completely ignore the fields that lay in between them.
 */
#define TKIT_YAML_SERIALIZE_IGNORE_BEGIN()
#define TKIT_YAML_SERIALIZE_IGNORE_END()
