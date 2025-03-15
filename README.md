# Sourcetrail

Sourcetrail is a free and open-source cross-platform source explorer that helps you get productive on unfamiliar source code.

[![Build](https://github.com/OpenSourceSourceTrail/Sourcetrail/actions/workflows/build.yml/badge.svg)](https://github.com/OpenSourceSourceTrail/Sourcetrail/actions/workflows/build.yml)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/01bfc8f1428b40f7bc674369cdba1b93)](https://app.codacy.com/gh/OpenSourceSourceTrail/Sourcetrail/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

__Links__
* [Download](https://github.com/OpenSourceSourceTrail/Sourcetrail/releases)
* [Quick Start Guide](DOCUMENTATION.md#getting-started)
* [Documentation](DOCUMENTATION.md)
* [Changelog](CHANGELOG.md)

!["Sourcetrail User Interface"](docs/readme/user_interface.png "Sourcetrail User Interface")

Sourcetrail is:
* free
* working offline
* operating on Windows, ~~macOS~~ and Linux
* supporting C/C++
* ~~offering an SDK ([SourcetrailDB](https://github.com/CoatiSoftware/SourcetrailDB)) to write custom language extensions~~

## Using Sourcetrail

To setup Sourcetrail on your machine, you can either download the respective build for your operating system from our list of [Releases](https://github.com/CoatiSoftware/Sourcetrail/releases) and install it on your machine, or use one of the following package managers:

After your installation is complete, follow our [Quick Start Guide](DOCUMENTATION.md#getting-started) to get to know Sourcetrail.

## How to Report Issues

You can post all your feature requests and bug reports on our [issue tracker](https://github.com/OpenSourceSourceTrail/Sourcetrail/issues).

### Reporting

Use the following template:

* platform version:
* Sourcetrail version:
* description of the problem:
* steps to reproduce the problem:

### Supporting

If you want to support a certain feature request or you have the same bug that another user already reported, please let us know:
* post a comment with "+1" to the issue

## How to Contribute

* Please read and follow the steps in [CONTRIBUTING.md](CONTRIBUTING.md) file.

# How to Build

Building Sourcetrail requires several dependencies to be in place on your machine. However, our CMake based setup allows to disable indexing support for specific languages which reduces the number of dependencies to a minimum.

## Building the Base Application

### Required Tools

* __CMake v3.23 (required for Windows, Linux and MacOS)__
    * __Reason__: Used to generate a build configuration for your build system
    * __Download__: https://cmake.org/download

* __Git (required for Windows, Linux and MacOS)__
    * __Reason__: Used for version control and to automatically generate the Sourcetrail version number from commits and tags
    * __Download__: https://git-scm.com/download
    * __Remarks__: Make sure `git` is added to your `PATH` environment variable before running CMake

* __Visual Studio (required for Windows)__
    * __Reason__: Used for building Sourcetrail
    * __Download__: https://visualstudio.microsoft.com/downloads/

### Required dependencies

* __Conan 2__
    * __Reason__: Package Manager
    * __Install__: pip3 install conan

* __Qt 6.8.2__
    * __Reason__: Used for rendering the GUI and for starting additional (indexer) processes.
    * __Prebuilt Download__: http://download.qt.io/official_releases/qt/
    * __aqt installer__: https://github.com/miurahr/aqtinstall

### Building

#### On Windows `Faced some problems with Conan2 and Windows`
* To set up your build environment run:
    ```
    $ pip install conan
    $ git clone --recurse-submodules https://github.com/OpenSourceSourceTrail/Sourcetrail.git
    $ cd Sourcetrail
    $ conan install . -s build_type=Release -of .conan/vc/ -b missing
    $ cmake --preset=vc_release
    $ cd ../build/Release/
    $ ninja
    ```

#### On Unix

* To set up your build environment run:
    ```
    $ pip install conan
    $ git clone  --recurse-submodules https://github.com/OpenSourceSourceTrail/Sourcetrail.git
    $ cd Sourcetrail
    $ conan install . -s build_type=Release -of .conan/gcc/ -b missing -pr:a .conan/gcc/profile
    $ cmake --preset=gnu_release
    $ cd ../build/Release/GNU
    $ ninja
    ```

### Running

* Run Sourcetrail from within the `bin` directory.

## Enable C/C++ Language Support

### Required dependencies

* __LLVM/Clang 19.1.7__
    * __Reason__: Used for running the preprocessor on the indexed source code, building and traversing an Abstract Syntax Tree and generating error messages.
    * __Building__: Make sure to check out the correct tag: `git checkout llvmorg-19.1.7`
    * __Building for Windows__: Follow [these steps](https://clang.llvm.org/get_started.html) to build the project. Run the cmake command exactly as described. Make sure to build with `-DLLVM_ENABLE_PROJECTS:STRING=clang -DLLVM_ENABLE_RTTI:BOOL=ON -DLLVM_TARGETS_TO_BUILD=host`.
    * __Building for Unix__: Follow this [installation guide](http://clang.llvm.org/docs/LibASTMatchersTutorial.html) to build the project. Make sure to build with `-DLLVM_ENABLE_PROJECTS:STRING=clang -DLLVM_ENABLE_RTTI:BOOL=ON -DCLANG_LINK_CLANG_DYLIB:BOOL=ON -DLLVM_LINK_LLVM_DYLIB:BOOL=ON -DLLVM_TARGETS_TO_BUILD=host`.

### Building

* Run CMake with these additional options:
    ```
    -DClang_DIR=<path/to/llvm_build>/lib/cmake/clang
    -DBUILD_CXX_LANGUAGE_PACKAGE=ON
    ```
* Build Sourcetrail as described [above](#building).


### How to Run the Tests

The automated test suite of Sourcetrail is powered by [GTest](https://github.com/google/googletest). To run the tests, simply execute the `ctest`.

### Special thanks
A special thanks for jetbrain for providing a license for clion. 

![Jetbrains](https://resources.jetbrains.com/storage/products/company/brand/logos/jb_beam.svg?_gl=1*1g15bg8*_ga*MzY0NDcyNy4xNjk2NjExMzg0*_ga_9J976DJZ68*MTcwNjcwNzIxNC40LjEuMTcwNjcwNzIyOS40NS4wLjA.&_ga=2.185029930.2038936796.1706702230-3644727.1696611384)

### License

Sourcetrail is licensed under the [GNU General Public License Version 3](LICENSE.txt).

### Trademark

The "Sourcetrail" name is a trademark owned by Coati Software and is not included within the assets licensed under the GNU GPLv3.
