package com.sourcetrail.indexer;

import com.github.javaparser.ParserConfiguration;
import com.github.javaparser.ParseResult;
import com.github.javaparser.StaticJavaParser;
import com.github.javaparser.ast.CompilationUnit;
import java.io.IOException;
import java.nio.file.Path;
import sourcetrail.SourcetrailCommon.IndexerCommand;
import sourcetrail.SourcetrailCommon.IntermediateStorage;

/**
 * Java AST indexer. Parses a single .java source file, walks the compilation unit
 * with {@link JavaCollector}, and emits an {@link IntermediateStorage} for the
 * engine to merge.
 *
 * <p>The collector emits one node per declaration (type, method, field, enum
 * constant, etc.), edges for every reference (extends / implements / calls /
 * type usages / annotations / imports), and source locations for GUI
 * highlighting.
 */
public final class JavaIndexer implements Indexer {

  private final ParserConfiguration configuration;

  public JavaIndexer() {
    this(new ParserConfiguration());
  }

  public JavaIndexer(ParserConfiguration configuration) {
    this.configuration = configuration;
  }

  @Override
  public IntermediateStorage index(IndexerCommand command) {
    if(command.getType() != IndexerCommand.CommandType.JAVA) {
      return IntermediateStorage.newBuilder().setNextId(1).build();
    }

    String path = command.getSourceFilePath();
    Path file = Path.of(path);

    try {
      ParserConfiguration config = configuration.setLanguageLevel(languageLevelOf(command.getLanguageStandard()));
      com.github.javaparser.JavaParser parser = new com.github.javaparser.JavaParser(config);
      ParseResult<CompilationUnit> result = parser.parse(file);

      if(result.isSuccessful() && result.getResult().isPresent()) {
        CompilationUnit cu = result.getResult().get();
        NameResolver resolver = new NameResolver(cu);
        Storage storage = new Storage(path);
        JavaCollector collector = new JavaCollector(storage, resolver);
        collector.visitRoot(cu);
        return storage.build();
      }

      // Parse failed: emit an empty storage so the engine marks this file as
      // incomplete rather than crashing the worker process.
      return IntermediateStorage.newBuilder().setNextId(1).build();
    } catch(IOException | RuntimeException e) {
      return IntermediateStorage.newBuilder().setNextId(1).build();
    }
  }

  /**
   * Map the source-group's language standard (a version number such as {@code "17"} or
   * {@code "21"}) to a JavaParser language level. Defaults to the newest supported level
   * so modern syntax (records, etc.) always parses.
   */
  private ParserConfiguration.LanguageLevel languageLevelOf(String standard) {
    if(standard == null || standard.isEmpty()) {
      return ParserConfiguration.LanguageLevel.JAVA_21;
    }
    try {
      int v = Integer.parseInt(standard.trim());
      switch(v) {
        case 8: return ParserConfiguration.LanguageLevel.JAVA_8;
        case 9: return ParserConfiguration.LanguageLevel.JAVA_9;
        case 10: return ParserConfiguration.LanguageLevel.JAVA_10;
        case 11: return ParserConfiguration.LanguageLevel.JAVA_11;
        case 14:
        case 15: return ParserConfiguration.LanguageLevel.JAVA_15;
        case 16: return ParserConfiguration.LanguageLevel.JAVA_16;
        case 17: return ParserConfiguration.LanguageLevel.JAVA_17;
        case 18: return ParserConfiguration.LanguageLevel.JAVA_18;
        case 19: return ParserConfiguration.LanguageLevel.JAVA_19;
        case 20: return ParserConfiguration.LanguageLevel.JAVA_20;
        default: break;
      }
    } catch(NumberFormatException ignored) {
      // non-numeric standard: use the default
    }
    return ParserConfiguration.LanguageLevel.JAVA_21;
  }
}
