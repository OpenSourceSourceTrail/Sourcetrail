package com.sourcetrail.indexer;

/**
 * Argv contract (matches src/app/indexer/main.cpp):
 * {@code <processId> --engine-endpoint <host:port> <sharedDataPath> <userDataPath> [logFilePath]}
 */
public final class Main {
  private Main() {}

  public static void main(String[] args) {
    if(args.length < 4 || !"--engine-endpoint".equals(args[1])) {
      System.err.println(
          "usage: <processId> --engine-endpoint <host:port> <sharedDataPath> <userDataPath> [logFilePath]");
      System.exit(1);
    }

    long processId = Long.parseLong(args[0]);
    String engineEndpoint = args[2];
    // args[3] sharedDataPath, args[4] userDataPath, args[5] logFilePath: unused by increment 3.

    new GrpcWorker(engineEndpoint, processId, new EmptyIndexer()).work();
  }
}
