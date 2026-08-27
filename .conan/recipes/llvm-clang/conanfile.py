import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy, get


class LlvmClangConan(ConanFile):
    """LLVM/Clang built exactly the way Sourcetrail's C/C++ indexer needs it.

    The indexer consumes LLVM through LLVM's *own* CMake package files
    (`find_package(Clang)` in src/lib/lib_cxx/CMakeLists.txt), so this recipe ships the
    upstream install tree untouched and disables CMakeDeps generation for itself.
    """

    name = "llvm-clang"
    version = "23.1.0"
    license = "Apache-2.0 WITH LLVM-exception"
    homepage = "https://llvm.org"
    url = "https://github.com/llvm/llvm-project"
    description = "LLVM/Clang with RTTI and the LLVM/clang-cpp dylibs, host target only"
    topics = ("llvm", "clang", "compiler", "libtooling")

    package_type = "shared-library"
    settings = "os", "arch", "compiler", "build_type"

    # The LLVM sources are ~1.4 GB compressed; never copy them into the build folder.
    no_copy_source = True

    def configure(self):
        # LLVM picks its own C++ standard; the consumer profile's cppstd=20 must not be
        # injected into this build.
        self.settings.rm_safe("compiler.cppstd")

    def layout(self):
        cmake_layout(self, src_folder="src")

    def build_requirements(self):
        self.tool_requires("ninja/[>=1.11]")

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        tc = CMakeToolchain(self, generator="Ninja")

        # The flag set Sourcetrail requires -- keep these in sync with README.md.
        tc.cache_variables["LLVM_ENABLE_PROJECTS"] = "clang"
        tc.cache_variables["LLVM_ENABLE_RTTI"] = True
        tc.cache_variables["CLANG_LINK_CLANG_DYLIB"] = True
        tc.cache_variables["LLVM_LINK_LLVM_DYLIB"] = True
        tc.cache_variables["LLVM_TARGETS_TO_BUILD"] = "host"

        # Packaging hygiene only: these trim build time and keep system libraries out of the
        # package's link interface. None of them contradicts the required flags above.
        tc.cache_variables["LLVM_INCLUDE_TESTS"] = False
        tc.cache_variables["LLVM_INCLUDE_BENCHMARKS"] = False
        tc.cache_variables["LLVM_INCLUDE_EXAMPLES"] = False
        tc.cache_variables["LLVM_ENABLE_ASSERTIONS"] = False
        tc.cache_variables["LLVM_ENABLE_TERMINFO"] = False
        tc.cache_variables["LLVM_ENABLE_LIBXML2"] = False
        tc.cache_variables["LLVM_ENABLE_ZLIB"] = False
        tc.cache_variables["LLVM_ENABLE_ZSTD"] = False
        tc.cache_variables["LLVM_ENABLE_LIBEDIT"] = False
        tc.cache_variables["CLANG_INCLUDE_TESTS"] = False

        tc.generate()

    def build(self):
        cmake = CMake(self)
        # The release tarball's CMake root is the llvm/ subdirectory, not the top level.
        cmake.configure(build_script_folder="llvm")
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, "LICENSE.TXT",
             src=os.path.join(self.source_folder, "llvm"),
             dst=os.path.join(self.package_folder, "licenses"))
        # NOTE: do not prune lib/clang/<major>/include -- Clang locates its builtin resource
        # headers relative to the loaded libclang-cpp, and nothing in Sourcetrail passes
        # -resource-dir. Removing them breaks indexing at runtime.

    def package_info(self):
        # Consumers must find LLVM's own ClangConfig.cmake / LLVMConfig.cmake, never a
        # Conan-generated stand-in: llvm_map_components_to_libnames(), LLVM_LINK_LLVM_DYLIB
        # and CLANG_LINK_CLANG_DYLIB all come from those files.
        self.cpp_info.set_property("cmake_find_mode", "none")
        self.cpp_info.builddirs = [
            os.path.join("lib", "cmake", "llvm"),
            os.path.join("lib", "cmake", "clang"),
        ]
