#pragma once

/**
 * These reflection macros are placeholder macros that will trigger reflection code generation scripts. These triggers
 * are set up with CMake, so the project must be built with it for it to work. Marking classes/structs with these macros
 * alone won't work either, as an extra step is needed: When listing the source files of your project, after calling the
 * CMake functions add_executable or add_library, you must call the CMake function tkit_register_for_reflection, with
 * the target and the source files the code generation scripts will have to scan to find the marks.
 *
 * The generated code will have information about the fields of any struct or class appriopriately marked, and will live
 * relative to this very folder. This means generated reflection code should appear next to this file. The output of the
 * reflection system will follow your folder structure to ensure no mismatches/overwrites happen. Here is an example:
 *
 * Assuming the folder structure follows:
 * `${workspaceFolder}/project-name/project-small-identifier/my_marked_classes.hpp`
 *
 * then, assuming a typical CMake project setup, the output file will be located in the following path:
 * `${workspaceFolder}/build/_deps/toolkit-src/toolkit/tkit/reflection/project-small-identifier/my_marked_classes.hpp`
 * and you may include it in your code as follows:
 * `#include "tkit/reflection/project-small-identifier/my_marked_classes.hpp"`
 */

/**
 * The main reflection macro, used to mark classes or structs required for reflection. Unmarked classes or structs will
 * be ignored. The macro expands to a friend statement so that the Reflect class may have access to private fields.
 */
#ifdef TKIT_ENABLE_REFLECTION
#    define TKIT_REFLECT_DECLARE(p_ClassName) friend class TKit::Reflect<p_ClassName>;
#else
#    define TKIT_REFLECT_DECLARE(p_ClassName)
#endif

/**
 * A pair of macros that will allow you to group fields in any way you prefer, so that later, when iterating over the
 * fields of a class, you may do so only within a subset of the fields.
 */
#define TKIT_REFLECT_GROUP_BEGIN(p_GroupName)
#define TKIT_REFLECT_GROUP_END()

/**
 * This pair of macros will allow you to completely ignore the fields that lay in between them.
 */
#define TKIT_REFLECT_IGNORE_BEGIN
#define TKIT_REFLECT_IGNORE_END

#define TKIT_REFLECT_FIELD_TYPE(p_Field) typename std::remove_cvref_t<decltype(p_Field)>::Type

namespace TKit
{
/**
 * @brief A placeholder Reflect class.

 * Code generation scripts will generate specializations of this class. The `Implemented`
 * variable controls if reflection is implemented for any class or struct.
 */
template <typename T> class Reflect
{
  public:
    static constexpr bool Implemented = false;
};
}; // namespace TKit
