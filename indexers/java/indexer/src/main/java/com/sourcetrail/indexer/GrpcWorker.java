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
import sourcetrail.IndexerWorker.PushIntermediateStorageResponse;
import sourcetrail.IndexerWorker.StatusReport;
import sourcetrail.IndexerWorker.WatchInterruptRequest;
import sourcetrail.IndexerWorkerServiceGrpc;
import sourcetrail.SourcetrailCommon.IndexerCommand;
import sourcetrail.SourcetrailCommon.IntermediateStorage;

/** Mirrors GrpcIndexer.cpp: pull -> report(start) -> index -> push -> report(finish), looped. */
public final class GrpcWorker {
  private static final Logger LOG = Logger.getLogger(GrpcWorker.class.getName());

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
    ManagedChannel channel = ManagedChannelBuilder.forTarget(engineEndpoint).usePlaintext().build();
    try {
      IndexerWorkerServiceGrpc.IndexerWorkerServiceBlockingStub stub =
          IndexerWorkerServiceGrpc.newBlockingStub(channel);
      IndexerWorkerServiceGrpc.IndexerWorkerServiceStub asyncStub = IndexerWorkerServiceGrpc.newStub(channel);

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

      LOG.info(processId + " GrpcIndexer starting up");

      while(!interruptReceived.get()) {
        PullCommandResponse pullResp;
        try {
          pullResp = stub.pullCommand(PullCommandRequest.newBuilder().setProcessId(processId).build());
        } catch(Exception e) {
          LOG.log(Level.SEVERE, processId + " PullCommand failed: " + e.getMessage(), e);
          break;
        }

        if(!pullResp.getCommandFound()) {
          LOG.info(processId + " no more commands, shutting down");
          break;
        }

        IndexerCommand command = pullResp.getCommand();

        stub.reportStatus(
            StatusReport.newBuilder()
                .setProcessId(processId)
                .setEvent(StatusReport.EventType.START_FILE)
                .setFilePath(command.getSourceFilePath())
                .build());

        LOG.info(processId + " indexing: " + command.getSourceFilePath());
        IntermediateStorage result = indexer.index(command);

        if(result != null && !interruptReceived.get()) {
          PushIntermediateStorageResponse pushResp;
          try {
            pushResp =
                stub.pushIntermediateStorage(
                    PushIntermediateStorageRequest.newBuilder()
                        .setProcessId(processId)
                        .setStorage(result)
                        .build());
          } catch(Exception e) {
            LOG.log(Level.SEVERE, processId + " PushIntermediateStorage failed: " + e.getMessage(), e);
          }
        }

        stub.reportStatus(
            StatusReport.newBuilder().setProcessId(processId).setEvent(StatusReport.EventType.FINISH_FILE).build());
      }

      stub.reportStatus(
          StatusReport.newBuilder().setProcessId(processId).setEvent(StatusReport.EventType.PROCESS_DONE).build());

      LOG.info(processId + " GrpcIndexer shut down");

      try {
        watchDone.await(2, TimeUnit.SECONDS);
      } catch(InterruptedException e) {
        Thread.currentThread().interrupt();
      }
    } finally {
      channel.shutdownNow();
    }
  }
}
