# C/C++ indexer plugin (Clang/LibTooling language package + worker executable) -----------------------------------------------------
if(BUILD_CXX_LANGUAGE_PACKAGE)
  add_subdirectory(cxx)
endif()
# Java indexer plugin (Maven-built, pure-JVM gRPC worker) -------------------------------------------------------------------------
if(BUILD_JAVA_INDEXER)
  add_subdirectory(java)
endif()
