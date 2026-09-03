package com.sourcetrail.indexer;

import com.github.javaparser.JavaParser;
import com.github.javaparser.ParseResult;
import com.github.javaparser.ParserConfiguration;
import com.github.javaparser.Problem;
import com.github.javaparser.ast.CompilationUnit;
import com.github.javaparser.resolution.TypeSolver;
import com.github.javaparser.symbolsolver.JavaSymbolSolver;
import java.nio.file.Path;
import sourcetrail.SourcetrailCommon.IndexerCommand;
import sourcetrail.SourcetrailCommon.IntermediateStorage;

/**
 * Java AST indexer. Parses a single .java source file, walks the compilation unit
 * with {@link JavaCollector}, and emits an {@link IntermediateStorage} for the
 * engine to merge.
 *
 * <p>Resolution is best-effort: a {@link JavaSymbolSolver} built from the command's classpath
 * resolves calls, field accesses and inheritance to real declarations, and whatever it cannot
 * resolve falls back to {@link NameResolver}'s lexical FQN and is marked
 * {@link Kinds#LOCATION_UNSOLVED} so the GUI shows it as unresolved rather than as a confident
 * wrong answer. A project with no configured classpath still produces a usable graph.
 *
 * <p>No failure escapes this class: a parse error or an exception mid-walk produces whatever was
 * collected so far, plus a {@code StorageError} and {@code complete = false} on the file, rather
 * than taking the worker process down.
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
    Storage storage = new Storage(path);

    try {
      TypeSolver typeSolver = TypeSolvers.forClassPaths(command.getClassPathsList());
      ParserConfiguration config = configuration.setLanguageLevel(languageLevelOf(command.getLanguageStandard()))
          .setSymbolResolver(new JavaSymbolSolver(typeSolver));

      ParseResult<CompilationUnit> result = new JavaParser(config).parse(Path.of(path));

      if(!result.isSuccessful() || result.getResult().isEmpty()) {
        for(Problem problem : result.getProblems()) {
          storage.error(problem.getVerboseMessage(), path);
        }
        if(result.getProblems().isEmpty()) {
          storage.error("Failed to parse file.", path);
        }
        storage.setFileComplete(false);
        return storage.build();
      }

      CompilationUnit cu = result.getResult().get();
      new JavaCollector(storage, new NameResolver(cu), typeSolver, path).visitRoot(cu);
      return storage.build();
    } catch(Exception e) {
      // Keep the partial graph: it is strictly better than nothing, and the error row plus
      // complete=false tells the engine (and the user) that this file is not fully indexed.
      storage.error(e.getClass().getSimpleName() + ": " + e.getMessage(), path);
      storage.setFileComplete(false);
      return storage.build();
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
