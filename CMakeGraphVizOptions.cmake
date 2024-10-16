# Use "--graphviz=sourcetrail.dot" cmake command option for dependency graph generation.
# Use "dot -Tpng -o sourcetrail.png sourcetrail.dot" to generate png image of the gpaph.

set(GRAPHVIZ_GENERATE_PER_TARGET TRUE)
set(GRAPHVIZ_IGNORE_TARGETS CppSQLite3)
set(GRAPHVIZ_GRAPH_NAME "sourcetrail")
set(GRAPHVIZ_EXECUTABLES TRUE)
set(GRAPHVIZ_SHARED_LIBS FALSE)
set(GRAPHVIZ_EXTERNAL_LIBS TRUE)
set(GRAPHVIZ_UNKNOWN_LIBS TRUE)
