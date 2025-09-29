![Tests Status](https://github.com/ismawno/toolkit/actions/workflows/tests.yml/badge.svg)

# Toolkit

Toolkit is a small library with C++ utilities I have developed and find essential in every project I start. The main additions of the library revolve around additional data structures, memory management, multithreading, and small utilities I find handy. Some of the presented functionality is already available in the STL but is re-implemented here for style, performance and control reasons.

## Features

The features of this library are divided into the following categories. Specific documentation for each of them can be found in the source code. The documentation can also be built with Doxygen.

All features that require explicit compilation can be disabled (and are by default) so you can choose any subset of this library you are going to use without having to build the whole project.

### Data structures

Located under the [container](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/container) folder, these are handy data-structures centered around minimizing the need for heap allocations (except for `TKit::DynamicArray`/`TKit::DynamicQueue`, which are just a re-implementation of their STL counterparts that fit my style better), including but not limited to:

- [array.hpp](https://github.com/ismawno/toolkit/blob/main/toolkit/tkit/container/array.hpp): A fixed size array implementation very similar to `std::array` that plays nicely with the style of the library and implements bound checking that can be easily stripped from distribution or release builds.

- [dynamic_array.hpp](https://github.com/ismawno/toolkit/blob/main/toolkit/tkit/container/dynamic_array.hpp): A dynamic size array implementation very similar to `std::vector` that plays nicely with the style of the library and implements bound checking that can be easily stripped from distribution or release builds.

- [static_array.hpp](https://github.com/ismawno/toolkit/blob/main/toolkit/tkit/container/static_array.hpp): A hybrid between `TKit::Array` and `TKit::DynamicArray`, it inherits almost all of the functionality of the later, but uses a fixed size buffer, meaning the array can be resized up to its fixed capacity. This is very handy as the memory usage of the array is very predictable and local, but provides an API that allows object emplacement, just like a dynamic array would.

- [storage.hpp](https://github.com/ismawno/toolkit/blob/main/toolkit/tkit/container/storage.hpp): A small storage unit that reserves enough memory locally for a specific type and allows its deferred construction and destruction. It shares the versatility `std::unique_ptr` offers when an object cannot be constructed immediately because of previous requirements or needs to be re-created constantly, but the memory access pattern is the same as if the object was allocated in-place instead of through a heap allocation.

### Memory

Located under the [memory](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/memory) folder, this set of features revolve around allocation strategies and centralization of the functions that perform heap allocations. The bulk of it are the 4 different memory allocators, implementing 3 different allocation strategies and a general-purpose allocator:

- [arena_allocator.hpp](https://github.com/ismawno/toolkit/blob/main/toolkit/tkit/memory/arena_allocator.hpp): Allocates a single contiguous memory block on creation and allows the user to perform very fast allocations up to the size of the main buffer. Deallocation is done globally, freeing all memory at once.

- [stack_allocator.hpp](https://github.com/ismawno/toolkit/blob/main/toolkit/tkit/memory/stack_allocator.hpp): Very similar to the `TKit::ArenaAllocator` but individual allocations can (and must) be deallocated in the reverse order they were allocated, at the cost of a small bookeeping overhead.

- [block_allocator.hpp](https://github.com/ismawno/toolkit/blob/main/toolkit/tkit/memory/block_allocator.hpp): Provides very fast allocation and deallocation of chunks of memory of a fixed size with no ordering constraints. Useful when repeated allocation for the same type of object is needed.

- [tier_allocator.hpp](https://github.com/ismawno/toolkit/blob/main/toolkit/tkit/memory/tier_allocator.hpp): General purpose allocator implemented as an extension of the `TKit::BlockAllocator` by providing different tiers that correspond to an allocation size at the cost of a very small indirection that involves inferring the tier of an allocation through the provided size.

### Multiprocessing

Located under the [multiprocessing](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/multiprocessing) folder, it provides many utilities regarding multithreading and parallel execution. Some of these features are the following:

- [task.hpp](https://github.com/ismawno/toolkit/blob/main/toolkit/tkit/multiprocessing/task.hpp): An object that wraps a general callable and provides a very simple and thread safe way to signal task completion.

- [task_manager.hpp](https://github.com/ismawno/toolkit/blob/main/toolkit/tkit/multiprocessing/task_manager.hpp): An abstract class providing an interface for a user-implemented system that handles task execution and management using the `TKit::ITask` interface. There is also a basic implementation that features sequential execution.

- [thread_pool.hpp](https://github.com/ismawno/toolkit/blob/main/toolkit/tkit/multiprocessing/thread_pool.hpp): An implementation of `TKit::ITaskManager` that features an efficient lock-free work-stealing thread pool.

- [for_each.hpp](https://github.com/ismawno/toolkit/blob/main/toolkit/tkit/multiprocessing/for_each.hpp): A utility function that partitions a for-loop into different tasks to be executed by a task manager, potentially in parallel.

### Preprocessor

Located under the [preprocessor](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/preprocessor) folder, it features some preprocessor utilities and readable macros to identify compiler and operating system.

### Profiling

Located under the [profiling](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/profiling) folder, it contains ways to measure code performance and elapsed time in between operations. It also features instrumentation through the tracy profiler, which can be optionally pulled as a dependency through cmake in case instrumentation is enabled.

### Code generation

Located under the [reflection](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/reflection) and [serialization](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/serialization) folders, these two modules contain special macros that allow the user to mark classes, structs or enums for reflection or serialization code generation. The C++ code is generated through special python scripts developed in the [convoy](https://github.com/ismawno/convoy) project that are triggered at build time through `CMake`. For marked object definitions to be visible, the files that contain the reflection or serialization marks must be listed in special `CMake` functions, such as `tkit_register_for_reflection` and `tkit_register_for_yaml_serialization`. The generated code will be located in the [tkit/reflection](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/reflection) and [tkit/serialization](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/serialization) folders following the user's directory structure.

### General utilities

Located under the [utils](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/utils) folder, these are general purpose utilities, such as logging and assert macros that can be stripped away on non-debug builds, math utilities, concepts, literals etc.

## Dependencies and Third-Party Libraries

All dependencies are always pulled conditionally on features or operating systems. If your use case do not enable features that require dependencies, none will be downloaded.

## Building

The building process is straightforward. Using `CMake`:

```sh
cmake --preset release
cmake --build --preset release
```
