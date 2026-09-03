package com.sourcetrail.indexer;

import com.github.javaparser.resolution.TypeSolver;
import com.github.javaparser.symbolsolver.resolution.typesolvers.CombinedTypeSolver;
import com.github.javaparser.symbolsolver.resolution.typesolvers.JarTypeSolver;
import com.github.javaparser.symbolsolver.resolution.typesolvers.JavaParserTypeSolver;
import com.github.javaparser.symbolsolver.resolution.typesolvers.ReflectionTypeSolver;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * Builds (and caches) the {@link TypeSolver} for a command's classpath.
 *
 * <p>A worker process indexes hundreds of files that all carry the identical
 * {@code IndexerCommand.class_paths}, and constructing a {@link JarTypeSolver} means reading a jar's
 * whole entry table. Caching on the classpath list turns that from per-file into per-run.
 */
final class TypeSolvers {
  private static final Map<List<String>, TypeSolver> Cache = new HashMap<>();

  private TypeSolvers() {}

  static synchronized TypeSolver forClassPaths(List<String> classPaths) {
    return Cache.computeIfAbsent(List.copyOf(classPaths), TypeSolvers::build);
  }

  private static TypeSolver build(List<String> classPaths) {
    // IGNORE_ALL: an unresolvable symbol is the normal case for a project with an incomplete
    // classpath, and the collector already has a lexical fallback for it. Letting the solver throw
    // would abort the whole file over one unknown type.
    CombinedTypeSolver combined = new CombinedTypeSolver(CombinedTypeSolver.ExceptionHandlers.IGNORE_ALL);
    combined.add(new ReflectionTypeSolver(false));

    for(String entry : classPaths) {
      if(entry == null || entry.isEmpty()) {
        continue;
      }
      Path path = Path.of(entry);
      try {
        if(Files.isDirectory(path)) {
          combined.add(new JavaParserTypeSolver(path));
        } else if(entry.toLowerCase(Locale.ROOT).endsWith(".jar") && Files.isRegularFile(path)) {
          combined.add(new JarTypeSolver(path));
        }
      } catch(Exception e) {
        // A missing or corrupt classpath entry degrades resolution; it must not fail the run.
      }
    }
    return combined;
  }
}
