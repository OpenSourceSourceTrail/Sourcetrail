# ${PROJECT_SOURCE_DIR}/indexers/cxx/lib/CMakeLists.txt
find_package(Clang REQUIRED)

# Clang's own version config only accepts an exact major.minor, so the floor is checked by hand here.
if(Clang_VERSION_MAJOR LESS 23)
  message(
    FATAL_ERROR "Sourcetrail's C/C++ indexer requires LLVM/Clang 23 or newer, found ${Clang_VERSION} at ${Clang_DIR}. "
                "Point Clang_DIR at a matching <llvm_build>/lib/cmake/clang.")
endif()

add_library(Sourcetrail_lib_cxx)
add_library(Sourcetrail::lib::lib_cxx ALIAS Sourcetrail_lib_cxx)

target_sources(
  Sourcetrail_lib_cxx
  PRIVATE data/indexer/IndexerCxx.cpp
          data/parser/cxx/name/CxxDeclName.cpp
          data/parser/cxx/name/CxxFunctionDeclName.cpp
          data/parser/cxx/name/CxxName.cpp
          data/parser/cxx/name/CxxQualifierFlags.cpp
          data/parser/cxx/name/CxxStaticFunctionDeclName.cpp
          data/parser/cxx/name/CxxTypeName.cpp
          data/parser/cxx/name/CxxVariableDeclName.cpp
          data/parser/cxx/name_resolver/CxxDeclNameResolver.cpp
          data/parser/cxx/name_resolver/CxxNameResolver.cpp
          data/parser/cxx/name_resolver/CxxSpecifierNameResolver.cpp
          data/parser/cxx/name_resolver/CxxTemplateArgumentNameResolver.cpp
          data/parser/cxx/name_resolver/CxxTemplateParameterStringResolver.cpp
          data/parser/cxx/name_resolver/CxxTypeNameResolver.cpp
          data/parser/cxx/ASTAction.cpp
          data/parser/cxx/ASTConsumer.cpp
          data/parser/cxx/CanonicalFilePathCache.cpp
          data/parser/cxx/ClangInvocationInfo.cpp
          data/parser/cxx/CommentHandler.cpp
          data/parser/cxx/CxxAstVisitor.cpp
          data/parser/cxx/CxxAstVisitorComponent.cpp
          data/parser/cxx/CxxAstVisitorComponentBraceRecorder.cpp
          data/parser/cxx/CxxAstVisitorComponentContext.cpp
          data/parser/cxx/CxxAstVisitorComponentDeclRefKind.cpp
          data/parser/cxx/CxxAstVisitorComponentImplicitCode.cpp
          data/parser/cxx/CxxAstVisitorComponentIndexer.cpp
          data/parser/cxx/CxxAstVisitorComponentTypeRefKind.cpp
          data/parser/cxx/CxxCompilationDatabaseSingle.cpp
          data/parser/cxx/CxxContext.cpp
          data/parser/cxx/CxxDiagnosticConsumer.cpp
          data/parser/cxx/CxxParser.cpp
          data/parser/cxx/CxxVerboseAstVisitor.cpp
          data/parser/cxx/GeneratePCHAction.cpp
          data/parser/cxx/PreprocessorCallbacks.cpp
          data/parser/cxx/SingleFrontendActionFactory.cpp
          data/parser/cxx/utilityClang.cpp
          project/CxxToolchainLocal.cpp
          LanguagePackageCxx.cpp)

# One include root, e.g. #include "data/parser/cxx/CxxParser.h".
target_include_directories(Sourcetrail_lib_cxx PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_include_directories(Sourcetrail_lib_cxx PRIVATE ${PROJECT_BINARY_DIR}/src/lib/lib)

target_include_directories(Sourcetrail_lib_cxx SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})

if(LLVM_LINK_LLVM_DYLIB)
  set(REQ_LLVM_LIBS LLVM)
else()
  llvm_map_components_to_libnames(REQ_LLVM_LIBS support core libdriver passes option)
  foreach(LLVM_TARGET IN LISTS LLVM_TARGETS_TO_BUILD)
    get_property(lib_deps GLOBAL PROPERTY "LLVMBUILD_LIB_DEPS_LLVM${LLVM_TARGET}CodeGen")
    list(APPEND REQ_LLVM_LIBS "LLVM${LLVM_TARGET}CodeGen")
    list(APPEND REQ_LLVM_LIBS "${lib_deps}")
    get_property(lib_deps GLOBAL PROPERTY "LLVMBUILD_LIB_DEPS_LLVM${LLVM_TARGET}AsmParser")
    if(NOT "${lib_deps}" STREQUAL "")
      list(APPEND REQ_LLVM_LIBS "LLVM${LLVM_TARGET}AsmParser")
      list(APPEND REQ_LLVM_LIBS "${lib_deps}")
    endif()
  endforeach()
  list(APPEND REQ_LLVM_LIBS "LLVMX86AsmParser")
endif()

if(CLANG_LINK_CLANG_DYLIB)
  set(CLANG_LIBRARIES clang-cpp)
else()
  set(CLANG_LIBRARIES
      clangASTMatchers
      clangFrontend
      clangSerialization
      clangDriver
      clangOptions
      clangTooling
      clangParse
      clangSema
      clangStaticAnalyzerFrontend
      clangStaticAnalyzerCheckers
      clangStaticAnalyzerCore
      clangAnalysis
      clangRewriteFrontend
      clangEdit
      clangAST
      clangLex
      clangBasic)
endif()

target_link_libraries(
  Sourcetrail_lib_cxx
  PUBLIC Sourcetrail::lib::core::utility::OrderedCache
  PRIVATE Sourcetrail::lib::core
          Sourcetrail::lib::core::utility::logging
          Sourcetrail::lib::core::utility::Migrator
          Sourcetrail::lib::core::utility::ScopedSwitcher
          Sourcetrail::lib::core::utility::TextAccess
          Sourcetrail::lib::core::utility::TextCodec
          Sourcetrail::lib::core::utility::utility
          Sourcetrail::lib::core::utility::profiling
          Sourcetrail::lib::core::utility::file::FileRegister
          Sourcetrail::lib::core::utility::file::FilePathFilter
          Sourcetrail::lib::core::utility::file::FileManager
          Sourcetrail::lib::core::utility::file::FileTree
          Sourcetrail::lib::core::utility::file::FileSystem
          Sourcetrail::lib::lib
          Sourcetrail::lib::messaging
          Sourcetrail::lib::scheduling
          ${REQ_LLVM_LIBS}
          ${CLANG_LIBRARIES}
          Boost::json)

if(WIN32)
  target_compile_definitions(Sourcetrail_lib_cxx PRIVATE _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS) # Due to Clang
  target_link_libraries(Sourcetrail_lib_cxx PUBLIC version)
endif()

# Project warning set. PRIVATE so it applies here and is not inflicted on consumers.
target_link_libraries(Sourcetrail_lib_cxx PRIVATE Sourcetrail::warnings)
