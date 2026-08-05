#${CMAKE_SOURCE_DIR}/java/CMakeLists.txt
# Builds the Java indexer plugin with Maven and stages the shaded jar + manifest under
# <build>/app/plugins/java/, where IndexerPluginRegistry::discover() finds it at runtime.
set(SR_JAVA_PLUGIN_DIR "${CMAKE_BINARY_DIR}/app/plugins/java")
set(SR_JAVA_INDEXER_JAR "${CMAKE_CURRENT_SOURCE_DIR}/indexer/target/sourcetrail_java_indexer.jar")

add_custom_target(
  Sourcetrail_java_indexer ALL
  COMMAND "${SR_MAVEN_EXECUTABLE}" -f "${CMAKE_CURRENT_SOURCE_DIR}/indexer/pom.xml" -q package
  COMMAND ${CMAKE_COMMAND} -E make_directory "${SR_JAVA_PLUGIN_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SR_JAVA_INDEXER_JAR}" "${SR_JAVA_PLUGIN_DIR}/sourcetrail_java_indexer.jar"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/indexer/manifest.xml" "${SR_JAVA_PLUGIN_DIR}/manifest.xml"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/indexer"
  COMMENT "Building Java indexer plugin with Maven"
  VERBATIM)
