![Tests Status](https://github.com/ismawno/toolkit/actions/workflows/tests.yml/badge.svg)
![Build Status](https://github.com/ismawno/toolkit/actions/workflows/build.yml/badge.svg)

# toolkit
This small toolkit library contains C++ utilities I have developed and find useful on almost every project I start. It is meant for personal use only, although I have tried my best to make it as user friendly as possible.

The main features of the library revolve around additional data structures, memory management, multithreading and overall small utilities I find handy.

## Features

The features this library has are split between into the following 6 categories. Specific socumentation for each of them can be found in the source code. The documentation can also be built with doxygen.

- [kit/core](https://github.com/ismawno/toolkit/toolkit/kit/core): General purpose utilities, such as aliases, C++ 20 concepts, literals and a simple logging system with macros.

- [kit/container](https://github.com/ismawno/toolkit/toolkit/kit/container): Some handy data structures, such a multi hash tuple, a resizable array with an internal buffer with fixed capacity and a storage class that allows deferring the construction of objects (nice alternative to a unique pointer in some cases).


- [kit/memory](https://github.com/ismawno/toolkit/toolkit/kit/memory): Two different memory allocators, general allocation functions, new/delete overloads to track global memory usage and a custom reference counting system.


- [kit/multiprocessing](https://github.com/ismawno/toolkit/toolkit/kit/multiprocessing): A task manager interface that works with the task class, representing small units of work, along with a specific imlementation of a thread pool that complies with such interface. A small for each helper function is also created that uses the task manager interface to divide chunks of a for loop into different tasks.


- [kit/profiling](https://github.com/ismawno/toolkit/toolkit/kit/profiling): A wrapper around the Tracy profiler macros to instrument and profile an application, along with some very simple classes to manage timespans.


- [kit/utilities](https://github.com/ismawno/toolkit/toolkit/kit/utilities): Small and very simple standalone functions that are just nice to have.


All of the required build setup is done through CMake, where I have also added some functions to help with compiler and linker flags setup that I find very useful, specially when controlling such flags for third-party libraries (which I am not sure is very "good practicey", but makes my life easier)

## Dependencies and third party libraries

I have tried to keep these to a minimum, many of them being platform specific or entirely optional. All dependencies should download and setup automatically with the CMake setup. Such dependencies are the following:

- [fmt](https://github.com/fmtlib/fmt): Because linux compilers lack the `std::format()` functionality, this library is needed in that platform to account for this. The formatting in the logging system is so sparse that I can get away with `std::format()` in Windows and MacOS.

- [tracy](https://github.com/wolfpld/tracy): This is the profiling library toolkit uses under the hood with its profiling interface. It will only be downloaded if the proper CMake options are toggled, so if you are not interested in profiling, this library wont be embedded in your build.

## Building

The building process is (fortunately) very straightforward. Create a `build` folder, `cd` to it and run `cmake ..`. All available toolkit options will be printed out, which are pretty self explanatory. If an inconsistent combination of these options were to be entered, a warning/error message will appear (or so I hope).

Compile then the project with your editor/IDE of choice, and run the tests to make sure everything works as expected. If that is the case, you are done!