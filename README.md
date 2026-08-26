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

## How it works

Sourcetrail is split into several processes that talk over gRPC:

| Binary | Role |
| --- | --- |
| `Sourcetrail` | Qt GUI. Owns no database; it supervises the engine and reads everything through it. |
| `Sourcetrail_engine` | Headless daemon. Owns the SQLite index and serves the GUI/CLI over gRPC. |
| `Sourcetrail_indexer` | Indexer worker process, spawned per indexing job. Built from `indexers/cxx/` only when `BUILD_CXX_LANGUAGE_PACKAGE` is on. |
| `Sourcetrail_cli` | Headless, Qt-free front end. |

Indexers are plugins: each ships a manifest under `<build>/app/plugins/<name>/` and the
engine discovers them at startup. The GUI only offers the languages the running engine
reports, so a build without the C/C++ package simply shows fewer project types.

## Using Sourcetrail

To setup Sourcetrail on your machine, you can either download the respective build for your operating system from our list of [Releases](https://github.com/OpenSourceSourceTrail/Sourcetrail/releases) and install it on your machine.

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
    * __Reason__: Package Manager. Pulls Boost, gRPC/protobuf, spdlog, fmt, SQLite and the rest.
    * __Install__: pip3 install conan

* __Qt 6.10__ (6.11 works too on Linux; aqtinstall 3.3.0 cannot fetch 6.11 for Windows)
    * __Reason__: Used for rendering the GUI and for starting additional (engine/indexer) processes.
    * __Prebuilt Download__: http://download.qt.io/official_releases/qt/
    * __aqt installer__ ([aqtinstall](https://github.com/miurahr/aqtinstall)):

        ```
        $ pip install aqtinstall
        $ aqt list-qt linux desktop            # available versions
        $ aqt install-qt linux desktop 6.10.3 linux_gcc_64 --outputdir ~/Qt
        ```

        The base package covers every Qt component Sourcetrail uses (Widgets, Sql, Test, Svg).
        Point CMake at it with `-DQt6_DIR=~/Qt/6.10.3/gcc_64/lib/cmake/Qt6`, or set it once in
        `CMakeUserPresets.json`. On Windows use `aqt install-qt windows desktop 6.10.3 win64_msvc2022_64`.

### Optional dependencies

* __Maven + JDK 21__
    * __Reason__: Builds the Java indexer plugin. CMake enables `BUILD_JAVA_INDEXER` automatically when `mvn` is on your `PATH`; pass `-DBUILD_JAVA_INDEXER=OFF` to skip it.

### Building

The tracked CMake presets are `ci_<gnu|clang|msvc>_release`, optionally suffixed with
`_build_cxx` for C/C++ indexing support. They all build into `<repo>/build/`. On Linux there
is also `gnu_debug`, which builds into `<repo>/build-debug/`.

#### On Windows `Faced some problems with Conan2 and Windows`
* To set up your build environment run:
    ```
    $ pip install conan
    $ git clone --recurse-submodules https://github.com/OpenSourceSourceTrail/Sourcetrail.git
    $ cd Sourcetrail
    $ conan install . --build missing -s build_type=Release -s compiler.cppstd=20 -c tools.cmake.cmaketoolchain:generator=Ninja -of .conan/msvc/
    $ cmake --preset=ci_msvc_release
    $ cmake --build build
    ```

#### On Unix

* To set up your build environment run:
    ```
    $ pip install conan
    $ git clone  --recurse-submodules https://github.com/OpenSourceSourceTrail/Sourcetrail.git
    $ cd Sourcetrail
    $ conan install . -s build_type=Release -of .conan/gcc/ -b missing -pr:a .conan/gcc/profile
    $ cmake --preset=ci_gnu_release
    $ cmake --build build
    ```
    That single GCC/Release `conan install` is the only one you need on Linux — do **not** run
    `conan install -s build_type=Debug`. For a Debug build of Sourcetrail itself, use the
    `gnu_debug` preset, which links the very same Release-built dependencies:
    ```
    $ cmake --preset=gnu_debug
    $ cmake --build build-debug
    ```

### Running

* All executables land in `build/app/`, next to the `data`, `user` and `plugins` directories
  they need. Run `build/app/Sourcetrail` from there; it starts `Sourcetrail_engine` itself.

## Enable C/C++ Language Support

### Required dependencies

* __LLVM/Clang 22.1.8__ (22 or newer is required; older releases do not provide the AST APIs the indexer uses)
    * __Reason__: Used for running the preprocessor on the indexed source code, building and traversing an Abstract Syntax Tree and generating error messages.
    * __Remarks__: It must be built with RTTI and the LLVM/clang-cpp dylibs — stock distro packages and the official LLVM release binaries are built with `LLVM_ENABLE_RTTI=OFF` and will not work.

#### Building LLVM with Conan (Linux, recommended)

`.conan/recipes/llvm-clang/` is a Conan 2 recipe that performs exactly the build described
below. Run:

```
$ ./scripts/build_llvm_conan.sh
```

It exports the recipe, builds `llvm-clang/22.1.8` into your Conan cache using the same
`.conan/gcc/profile` as the rest of the project, and symlinks `<repo>/external` at the
resulting package — which is where the `_build_cxx` presets already look for `Clang_DIR`.
The first run compiles LLVM from source and takes hours; afterwards it is a cache hit. This
is a separate `conan install` (into `.conan/llvm/`) and does not affect the main dependency
set in `.conan/gcc/`.

To skip the hours-long first build, download the package CI already published and restore it
into your Conan cache — the script then resolves it as a cache hit in seconds:

```
$ gh release download llvm-clang-22.1.8 -p 'llvm-clang-22.1.8-linux-x86_64.tgz'
$ conan cache restore llvm-clang-22.1.8-linux-x86_64.tgz
$ ./scripts/build_llvm_conan.sh          # cache hit; creates the external/ symlink
```

That asset is produced by `.github/workflows/llvm.yml`, which is also what CI consumes.

#### Building LLVM by hand

* __Building__: Make sure to check out the correct tag: `git checkout llvmorg-22.1.8`
* __Building for Windows__: Follow [these steps](https://clang.llvm.org/get_started.html) to build the project. Run the cmake command exactly as described. Make sure to build with `-DLLVM_ENABLE_PROJECTS:STRING=clang -DLLVM_ENABLE_RTTI:BOOL=ON -DLLVM_TARGETS_TO_BUILD=host`.
* __Building for Unix__: Follow this [installation guide](http://clang.llvm.org/docs/LibASTMatchersTutorial.html) to build the project. Make sure to build with `-DLLVM_ENABLE_PROJECTS:STRING=clang -DLLVM_ENABLE_RTTI:BOOL=ON -DCLANG_LINK_CLANG_DYLIB:BOOL=ON -DLLVM_LINK_LLVM_DYLIB:BOOL=ON -DLLVM_TARGETS_TO_BUILD=host`. These are the same flags the Conan recipe uses, so the two paths are interchangeable.

### Building

* Use the `_build_cxx` preset variant:
    ```
    $ cmake --preset=ci_gnu_release_build_cxx
    $ cmake --build build
    ```
    The preset defaults `Clang_DIR` to `<repo>/external/lib/cmake/clang/`, which
    `scripts/build_llvm_conan.sh` populates. With a hand-built LLVM elsewhere, pass
    `-DClang_DIR=<path/to/llvm_build>/lib/cmake/clang`. On any other preset, add
    `-DBUILD_CXX_LANGUAGE_PACKAGE=ON -DClang_DIR=...` by hand.


### How to Run the Tests

The automated test suite of Sourcetrail is powered by [GTest](https://github.com/google/googletest).
The `ci_*` presets already enable the unit, GUI and integration tests; on a custom configuration
turn them on with `-DENABLE_UNIT_TEST=ON -DENABLE_GUI_TEST=ON -DENABLE_INTEGRATION_TEST=ON`.
Run them with:

```
$ ctest --test-dir build
```

### Special thanks
A special thanks for jetbrain for providing a license for clion. 

![Jetbrains](https://resources.jetbrains.com/storage/products/company/brand/logos/jb_beam.svg?_gl=1*1g15bg8*_ga*MzY0NDcyNy4xNjk2NjExMzg0*_ga_9J976DJZ68*MTcwNjcwNzIxNC40LjEuMTcwNjcwNzIyOS40NS4wLjA.&_ga=2.185029930.2038936796.1706702230-3644727.1696611384)

### License

Sourcetrail is licensed under the [GNU General Public License Version 3](LICENSE.txt).

### Trademark

The "Sourcetrail" name is a trademark owned by Coati Software and is not included within the assets licensed under the GNU GPLv3.
