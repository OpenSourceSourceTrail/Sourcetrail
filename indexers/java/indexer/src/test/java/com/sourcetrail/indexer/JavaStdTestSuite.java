package com.sourcetrail.indexer;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import org.junit.jupiter.api.io.TempDir;
import sourcetrail.SourcetrailCommon.IndexerCommand;

/**
 * Abstract base for per-LTS Java indexer test suites.
 *
 * <p>Each concrete subclass pins one Java LTS version via {@link #standard()}, mirroring the way
 * {@code CxxParser11TestSuite}, {@code CxxParser17TestSuite}, etc. each pin a {@code -std=c++NN}
 * flag. The {@link #index} helpers write a code snippet to a temp file, run
 * {@link JavaIndexer#index} with the chosen language level, and hand the result to
 * {@link TestStorage#create} — so individual {@code @Test} methods read exactly like their C++
 * counterparts: index a snippet, assert on a bin.
 *
 * <p>The language-level mapping is performed by {@code JavaIndexer.languageLevelOf}. It honours
 * {@code "8"}, {@code "9"}, {@code "10"}, {@code "11"}, {@code "14"}–{@code "20"}, and
 * {@code "21"} (the default for anything else). The four LTS levels used by the concrete suites
 * are {@code "8"}, {@code "11"}, {@code "17"}, and {@code "21"}.
 */
abstract class JavaStdTestSuite {
  private final JavaIndexer indexer = new JavaIndexer();

  @TempDir
  Path workspace;

  /**
   * Returns the Java LTS level under test as the {@code language_standard} field of
   * {@link IndexerCommand}: one of {@code "8"}, {@code "11"}, {@code "17"}, {@code "21"}.
   */
  protected abstract String standard();

  /**
   * Indexes {@code code} written to {@code Test.java} with no extra classpath entries.
   */
  protected TestStorage index(String code) throws IOException {
    return index(code, "Test.java", List.of());
  }

  /**
   * Indexes {@code code} written to {@code workspace/fileName} with the supplied classpath.
   * Parent directories are created if they do not yet exist.
   */
  protected TestStorage index(String code, String fileName, List<String> classPaths) throws IOException {
    Path file = workspace.resolve(fileName);
    Files.createDirectories(file.getParent());
    Files.writeString(file, code);
    return TestStorage.create(
        indexer.index(IndexerCommand.newBuilder()
            .setType(IndexerCommand.CommandType.JAVA)
            .setSourceFilePath(file.toString())
            .setLanguageStandard(standard())
            .addAllClassPaths(classPaths)
            .build()));
  }
}
