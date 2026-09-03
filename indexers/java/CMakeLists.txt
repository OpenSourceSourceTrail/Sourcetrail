#${PROJECT_SOURCE_DIR}/java/CMakeLists.txt
# Builds the Java indexer plugin with Maven and stages the shaded jar + manifest under
# <build>/app/plugins/java/, where IndexerPluginRegistry::discover() finds it at runtime.
if(NOT SR_MAVEN_EXECUTABLE)
  message(
    FATAL_ERROR
      "BUILD_JAVA_INDEXER is ON but no 'mvn' was found on PATH. Install Maven and JDK 21, or configure with -DBUILD_JAVA_INDEXER=OFF."
  )
endif()

set(SR_JAVA_PLUGIN_DIR "${PROJECT_BINARY_DIR}/app/plugins/java")
set(SR_JAVA_INDEXER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/indexer")
set(SR_JAVA_INDEXER_JAR "${SR_JAVA_INDEXER_DIR}/target/sourcetrail_java_indexer.jar")
set(SR_JAVA_STAGED_JAR "${SR_JAVA_PLUGIN_DIR}/sourcetrail_java_indexer.jar")

# The jar's inputs, so Maven only runs when something it consumes actually changed. The protos are
# in here because pom.xml points protoSourceRoot at src/lib/proto and regenerates the stubs.
file(GLOB_RECURSE SR_JAVA_INDEXER_SOURCES CONFIGURE_DEPENDS "${SR_JAVA_INDEXER_DIR}/src/*.java")
file(GLOB SR_JAVA_INDEXER_PROTOS CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/lib/proto/*.proto")

add_custom_command(
  OUTPUT "${SR_JAVA_INDEXER_JAR}"
  COMMAND "${SR_MAVEN_EXECUTABLE}" -f "${SR_JAVA_INDEXER_DIR}/pom.xml" -q package
  DEPENDS ${SR_JAVA_INDEXER_SOURCES} ${SR_JAVA_INDEXER_PROTOS} "${SR_JAVA_INDEXER_DIR}/pom.xml"
  WORKING_DIRECTORY "${SR_JAVA_INDEXER_DIR}"
  COMMENT "Building Java indexer plugin with Maven"
  VERBATIM)

add_custom_command(
  OUTPUT "${SR_JAVA_STAGED_JAR}" "${SR_JAVA_PLUGIN_DIR}/manifest.xml"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${SR_JAVA_PLUGIN_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SR_JAVA_INDEXER_JAR}" "${SR_JAVA_STAGED_JAR}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SR_JAVA_INDEXER_DIR}/manifest.xml"
          "${SR_JAVA_PLUGIN_DIR}/manifest.xml"
  DEPENDS "${SR_JAVA_INDEXER_JAR}" "${SR_JAVA_INDEXER_DIR}/manifest.xml"
  COMMENT "Staging Java indexer plugin into app/plugins/java"
  VERBATIM)

add_custom_target(Sourcetrail_java_indexer ALL DEPENDS "${SR_JAVA_STAGED_JAR}" "${SR_JAVA_PLUGIN_DIR}/manifest.xml")
