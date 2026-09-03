package com.sourcetrail.indexer;

import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import io.grpc.stub.StreamObserver;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.logging.Level;
import java.util.logging.Logger;
import sourcetrail.IndexerWorker.InterruptEvent;
import sourcetrail.IndexerWorker.PullCommandRequest;
import sourcetrail.IndexerWorker.PullCommandResponse;
import sourcetrail.IndexerWorker.PushIntermediateStorageRequest;
import sourcetrail.IndexerWorker.StatusReport;
import sourcetrail.IndexerWorker.WatchInterruptRequest;
import sourcetrail.IndexerWorkerServiceGrpc;
import sourcetrail.SourcetrailCommon.IndexerCommand;
import sourcetrail.SourcetrailCommon.IntermediateStorage;

/**
 * Mirrors {@code src/features/indexing/logic/grpc/GrpcIndexer.cpp}:
 * pull -> report(START_FILE) -> index -> push -> report(FINISH_FILE), looped until the engine
 * closes the command queue or an interrupt arrives, then report(PROCESS_DONE).
 */
public final class GrpcWorker {
  private static final Logger LOG = Logger.getLogger(GrpcWorker.class.getName());

  /**
   * gRPC-Java defaults to a 4 MB inbound limit, which a large translation unit blows through as
   * RESOURCE_EXHAUSTED. The C++ side sets grpc_indexer::UnlimitedMessageSize (-1) on both channel
   * and server; Integer.MAX_VALUE is the Java equivalent.
   */
  private static final int UnlimitedMessageSize = Integer.MAX_VALUE;

  private static final long WatcherShutdownTimeoutSeconds = 2;

  private final String engineEndpoint;
  private final long processId;
  private final Indexer indexer;
  private final AtomicBoolean interruptReceived = new AtomicBoolean(false);

  public GrpcWorker(String engineEndpoint, long processId, Indexer indexer) {
    this.engineEndpoint = engineEndpoint;
    this.processId = processId;
    this.indexer = indexer;
  }

  public void work() {
    work(ManagedChannelBuilder.forTarget(engineEndpoint)
        .usePlaintext()
        .maxInboundMessageSize(UnlimitedMessageSize)
        .build());
  }

  /** Channel-injecting entry point, so tests can drive the loop over an in-process server. */
  void work(ManagedChannel channel) {
    long files = 0;
    long pullNanos = 0;
    long indexNanos = 0;
    long pushNanos = 0;

    try {
      IndexerWorkerServiceGrpc.IndexerWorkerServiceBlockingStub stub =
          IndexerWorkerServiceGrpc.newBlockingStub(channel);
      CountDownLatch watchDone = watchInterrupt(IndexerWorkerServiceGrpc.newStub(channel));

      LOG.info(processId + " GrpcIndexer starting up");

      while(!interruptReceived.get()) {
        PullCommandResponse pullResp;
        long started = System.nanoTime();
        try {
          pullResp = stub.pullCommand(PullCommandRequest.newBuilder().setProcessId(processId).build());
        } catch(RuntimeException e) {
          LOG.log(Level.SEVERE, processId + " PullCommand failed: " + e.getMessage(), e);
          break;
        } finally {
          pullNanos += System.nanoTime() - started;
        }

        if(!pullResp.getCommandFound()) {
          // An empty queue is not the end of the run: the engine parks the call for ~1s on our
          // behalf and only sets queue_closed once no more commands will ever arrive. Exiting on a
          // transient miss would burn one of the three respawns TaskBuildIndex allows per worker.
          if(pullResp.getQueueClosed()) {
            LOG.info(processId + " no more commands, shutting down");
            break;
          }
          continue;
        }

        IndexerCommand command = pullResp.getCommand();

        stub.reportStatus(StatusReport.newBuilder()
            .setProcessId(processId)
            .setEvent(StatusReport.EventType.START_FILE)
            .setFilePath(command.getSourceFilePath())
            .build());

        LOG.info(processId + " indexing: " + command.getSourceFilePath());
        started = System.nanoTime();
        IntermediateStorage result = indexer.index(command);
        indexNanos += System.nanoTime() - started;
        files++;

        if(result == null || interruptReceived.get()) {
          continue;
        }

        boolean indexed;
        started = System.nanoTime();
        try {
          stub.pushIntermediateStorage(PushIntermediateStorageRequest.newBuilder()
              .setProcessId(processId)
              .setStorage(result)
              .build());
          indexed = true;
        } catch(RuntimeException e) {
          LOG.log(Level.SEVERE, processId + " PushIntermediateStorage failed: " + e.getMessage(), e);
          indexed = false;
        } finally {
          pushNanos += System.nanoTime() - started;
        }

        // Only a successfully pushed file gets a FINISH_FILE. Withholding it leaves the file
        // registered as in-flight, so the engine routes it through drainAndGetCrashedFiles() and
        // the user sees an error instead of a silently empty file.
        if(indexed) {
          stub.reportStatus(StatusReport.newBuilder()
              .setProcessId(processId)
              .setEvent(StatusReport.EventType.FINISH_FILE)
              .build());
        }
      }

      stub.reportStatus(StatusReport.newBuilder()
          .setProcessId(processId)
          .setEvent(StatusReport.EventType.PROCESS_DONE)
          .build());

      LOG.info(processId + " GrpcIndexer shut down");

      try {
        watchDone.await(WatcherShutdownTimeoutSeconds, TimeUnit.SECONDS);
      } catch(InterruptedException e) {
        Thread.currentThread().interrupt();
      }
    } finally {
      channel.shutdownNow();
      // TaskBuildIndex greps the worker's captured output for this line; it is the only thing the
      // worker is allowed to print when logging is off.
      System.err.printf(
          "INDEXER_TIMING process=%d files=%d parse_ms=%.1f serialize_ms=%.1f push_ms=%.1f pull_ms=%.1f%n",
          processId, files, indexNanos / 1e6, 0.0, pushNanos / 1e6, pullNanos / 1e6);
    }
  }

  private CountDownLatch watchInterrupt(IndexerWorkerServiceGrpc.IndexerWorkerServiceStub asyncStub) {
    CountDownLatch watchDone = new CountDownLatch(1);
    asyncStub.watchInterrupt(
        WatchInterruptRequest.newBuilder().setProcessId(processId).build(),
        new StreamObserver<InterruptEvent>() {
          @Override
          public void onNext(InterruptEvent event) {
            if(event.getInterrupted()) {
              LOG.info(processId + " received interrupt via gRPC");
              interruptReceived.set(true);
            }
          }

          @Override
          public void onError(Throwable t) {
            watchDone.countDown();
          }

          @Override
          public void onCompleted() {
            watchDone.countDown();
          }
        });
    return watchDone;
  }
}
