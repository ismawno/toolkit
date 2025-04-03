![Tests Status](https://github.com/ismawno/toolkit/actions/workflows/tests.yml/badge.svg)

# Toolkit

Toolkit is a small library with C++ utilities I have developed and find useful in almost every project I start. It is meant for personal use only, although I have tried my best to make it as user-friendly as possible.

The main features of the library revolve around additional data structures, memory management, multithreading, and small utilities I find handy.

## Features

The features of this library are divided into the following six categories. Specific documentation for each of them can be found in the source code. The documentation can also be built with Doxygen.

- [tkit/utils](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/utils): General-purpose utilities, such as aliases, C++20 concepts, literals, and a simple logging system with macros.

- [tkit/container](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/container): Handy data structures, such as a resizable array with an internal buffer of fixed capacity, a weak, non owning array, and a storage class that allows deferring the construction of objects (a nice alternative to a unique pointer in some cases).

- [tkit/memory](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/memory): Two different memory allocators, general allocation functions, `new`/`delete` overloads to track global memory usage, and a custom reference counting system.

- [tkit/multiprocessing](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/multiprocessing): A task manager interface that works with the task class, representing small units of work, along with a specific implementation of a thread pool that complies with this interface. A small `for_each` helper function is also provided, which uses the task manager interface to divide chunks of a `for` loop into different tasks.

- [tkit/profiling](https://github.com/ismawno/toolkit/tree/main/toolkit/tkit/profiling): A wrapper around the Tracy profiler macros to instrument and profile an application, along with some very simple classes to manage timespans.

Toolkit also features some small code generation scripts that can scan `.hpp` or `.cpp` files of your choosing, and generate reflection code for every marked `class` or `struct` they see. The script [reflect.py](https://github.com/ismawno/toolkit/blob/main/codegen/reflect.py) is a command line script that is in charge of this functionality. Use the `-h` or `--help` flag to learn more about its capabilities.
All of the required build setup is done through CMake, where I have also added some functions to help with compiler and linker flags setup that I find very useful, especially when controlling such flags for third-party libraries (which may not be very "best practicey" but makes my life easier).

## Dependencies and Third-Party Libraries

I have tried to keep dependencies to a minimum, many of them being platform-specific or entirely optional. All dependencies should download and set up automatically with the CMake setup. These dependencies are as follows:

- [fmt](https://github.com/fmtlib/fmt): Since Linux compilers lack the `std::format()` functionality, this library is required on that platform. The formatting in the logging system is sparse enough that I can use `std::format()` on Windows and macOS.

- [tracy](https://github.com/wolfpld/tracy): This is the profiling library used under the hood with the profiling interface. It will only be downloaded if the proper CMake options are toggled. If you are not interested in profiling, this library will not be embedded in your build.

- [yaml-cpp](https://github.com/jbeder/yaml-cpp): This library is used to parse configuration files and help with serialization. It will only be embedded if the proper CMake options are toggled.

- [catch2](https://github.com/catchorg/Catch2.git): This library is used for testing. It will only be used if building the tests.

## Building

The building process is (fortunately) very straightforward. Because of how much I hate how the CMake cache works, I have left some python building scripts in the [setup](https://github.com/ismawno/toolkit/tree/main/setup) folder.

The reason behind this is that CMake sometimes stores some variables in cache that you may not want to persist. This results in some default values for variables being only relevant if the variable itself is not already stored in cache. The problem with this is that I feel it is very easy to lose track of what configuration is being built unless I type in all my CMake flags explicitly every time I build the project, and that is just unbearable. Hence, these python scripts provide flags with reliable defaults stored in a `build.ini` file that are always applied unless explicitly changed with a command line argument.

Specifically, the [build.py](https://github.com/ismawno/toolkit/blob/main/setup/build.py) file, when executed from root, will handle the entire CMake execution process for you. You can enter `python setup/build.py -h` to see the available options.

If you prefer using CMake directly, that's perfectly fine as well. Create a `build` folder, `cd` into it, and run `cmake ..`. All available Toolkit options will be printed out, and they are self-explanatory. If an inconsistent combination of these options is entered, a warning or error message should appear (or so I hope).

Then compile the project with your editor/IDE of choice, and run the tests to make sure everything works as expected. If that is the case, you are done!
