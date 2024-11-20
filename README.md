![Tests Status](https://github.com/ismawno/toolkit/actions/workflows/tests.yml/badge.svg)
![Build Status](https://github.com/ismawno/toolkit/actions/workflows/build.yml/badge.svg)

# toolkit

*Toolkit* is a small library with C++ utilities I have developed and find useful in almost every project I start. It is meant for personal use only, although I have tried my best to make it as user-friendly as possible.

The main features of the library revolve around additional data structures, memory management, multithreading, and small utilities I find handy.

## Features

The features of this library are divided into the following six categories. Specific documentation for each of them can be found in the source code. The documentation can also be built with Doxygen.

- [kit/core](https://github.com/ismawno/toolkit/tree/main/toolkit/kit/core): General-purpose utilities, such as aliases, C++20 concepts, literals, and a simple logging system with macros.

- [kit/container](https://github.com/ismawno/toolkit/tree/main/toolkit/kit/container): Handy data structures, such as a multi-hash tuple, a resizable array with an internal buffer of fixed capacity, and a storage class that allows deferring the construction of objects (a nice alternative to a unique pointer in some cases).

- [kit/memory](https://github.com/ismawno/toolkit/tree/main/toolkit/kit/memory): Two different memory allocators, general allocation functions, `new`/`delete` overloads to track global memory usage, and a custom reference counting system.

- [kit/multiprocessing](https://github.com/ismawno/toolkit/tree/main/toolkit/kit/multiprocessing): A task manager interface that works with the task class, representing small units of work, along with a specific implementation of a thread pool that complies with this interface. A small `for_each` helper function is also provided, which uses the task manager interface to divide chunks of a `for` loop into different tasks.

- [kit/profiling](https://github.com/ismawno/toolkit/tree/main/toolkit/kit/profiling): A wrapper around the Tracy profiler macros to instrument and profile an application, along with some very simple classes to manage timespans.

- [kit/utilities](https://github.com/ismawno/toolkit/tree/main/toolkit/kit/utilities): Small and very simple standalone functions that are just nice to have.

All of the required build setup is done through CMake, where I have also added some functions to help with compiler and linker flags setup that I find very useful, especially when controlling such flags for third-party libraries (which may not be very "best practicey" but makes my life easier).

## Dependencies and Third-Party Libraries

I have tried to keep dependencies to a minimum, many of them being platform-specific or entirely optional. All dependencies should download and set up automatically with the CMake setup. These dependencies are as follows:

- [fmt](https://github.com/fmtlib/fmt): Since Linux compilers lack the `std::format()` functionality, this library is required on that platform. The formatting in the logging system is sparse enough that I can use `std::format()` on Windows and macOS.

- [tracy](https://github.com/wolfpld/tracy): This is the profiling library used under the hood with the profiling interface. It will only be downloaded if the proper CMake options are toggled. If you are not interested in profiling, this library will not be embedded in your build.

## Building

The building process is (fortunately) very straightforward. Create a `build` folder, `cd` into it, and run `cmake ..`. All available *toolkit* options will be printed out, and they are self-explanatory. If an inconsistent combination of these options is entered, a warning or error message should appear (or so I hope).

Then compile the project with your editor/IDE of choice, and run the tests to make sure everything works as expected. If that is the case, you are done!
