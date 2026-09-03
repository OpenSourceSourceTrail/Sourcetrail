package com.sourcetrail.indexer;

import java.io.IOException;
import java.util.logging.FileHandler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

/**
 * Argv contract (matches indexers/cxx/indexer/main.cpp):
 * {@code <processId> --engine-endpoint <host:port> <sharedDataPath> <userDataPath> [logFilePath]}
 */
public final class Main {
  /** Root of every logger in this worker; configured once, in {@link #configureLogging}. */
  static final String LOGGER_NAME = "com.sourcetrail.indexer";

  private Main() {}

  public static void main(String[] args) {
    // The engine always passes 5 arguments, 6 with verbose indexer logging enabled.
    if(args.length < 5 || !"--engine-endpoint".equals(args[1])) {
      System.err.println(
          "usage: <processId> --engine-endpoint <host:port> <sharedDataPath> <userDataPath> [logFilePath]");
      System.exit(1);
    }

    long processId;
    try {
      processId = Long.parseLong(args[0]);
    } catch(NumberFormatException e) {
      System.err.println("invalid process id: " + args[0]);
      System.exit(1);
      return;
    }

    String engineEndpoint = args[2];
    // args[3] sharedDataPath and args[4] userDataPath exist for parity with the C++ worker, which
    // needs them to locate ApplicationSettings. This worker reads no settings file.
    String logFilePath = args.length >= 6 ? args[5] : "";

    configureLogging(logFilePath, processId);

    new GrpcWorker(engineEndpoint, processId, new JavaIndexer()).work();
  }

  /**
   * The engine captures the worker's stdout/stderr and greps it for {@code INDEXER_TIMING}, so
   * anything else on those streams is noise. Mirror the C++ worker: log to the given file when the
   * engine asked for verbose indexer logging, and stay completely silent otherwise.
   */
  private static void configureLogging(String logFilePath, long processId) {
    Logger root = Logger.getLogger(LOGGER_NAME);
    root.setUseParentHandlers(false);

    if(logFilePath == null || logFilePath.isEmpty()) {
      root.setLevel(Level.OFF);
      return;
    }

    try {
      FileHandler handler = new FileHandler(logFilePath, true);
      handler.setFormatter(new SimpleFormatter());
      handler.setLevel(Level.ALL);
      root.addHandler(handler);
      root.setLevel(Level.ALL);
    } catch(IOException | SecurityException e) {
      // A missing log directory must not take the worker down; index silently instead.
      root.setLevel(Level.OFF);
    }
  }
}
