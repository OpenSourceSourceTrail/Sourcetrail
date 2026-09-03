package com.sourcetrail.indexer;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import io.grpc.ManagedChannel;
import io.grpc.Server;
import io.grpc.Status;
import io.grpc.inprocess.InProcessChannelBuilder;
import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.stub.StreamObserver;
import java.io.IOException;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;
import sourcetrail.IndexerWorker.InterruptEvent;
import sourcetrail.IndexerWorker.PullCommandRequest;
import sourcetrail.IndexerWorker.PullCommandResponse;
import sourcetrail.IndexerWorker.PushIntermediateStorageRequest;
import sourcetrail.IndexerWorker.PushIntermediateStorageResponse;
import sourcetrail.IndexerWorker.StatusReport;
import sourcetrail.IndexerWorker.StatusReportResponse;
import sourcetrail.IndexerWorker.WatchInterruptRequest;
import sourcetrail.IndexerWorkerServiceGrpc;
import sourcetrail.SourcetrailCommon.IndexerCommand;
import sourcetrail.SourcetrailCommon.IntermediateStorage;

/**
 * Drives {@link GrpcWorker} against an in-process fake of the engine's
 * {@code IndexerWorkerServiceImpl}, asserting the protocol it must match
 * ({@code src/features/indexing/logic/grpc/GrpcIndexer.cpp}).
 *
 * <p>The empty-queue case is the one that matters most: the engine parks a PullCommand for ~1s and
 * answers {@code command_found=false, queue_closed=false} when nothing is ready yet. A worker that
 * treats that as "done" exits, and TaskBuildIndex abandons it after three fast respawns.
 */
class GrpcWorkerTest {
  private Server server;
  private ManagedChannel channel;

  @AfterEach
  void tearDown() {
    if(channel != null) {
      channel.shutdownNow();
    }
    if(server != null) {
      server.shutdownNow();
    }
  }

  /** Scripted engine: answers PullCommand from a queue, records every StatusReport it receives. */
  private static final class FakeEngine extends IndexerWorkerServiceGrpc.IndexerWorkerServiceImplBase {
    final Deque<PullCommandResponse> pulls = new ArrayDeque<>();
    final List<StatusReport> reports = new ArrayList<>();
    final AtomicInteger pullCount = new AtomicInteger();
    final AtomicInteger pushCount = new AtomicInteger();
    boolean failPush;

    @Override
    public void pullCommand(PullCommandRequest request, StreamObserver<PullCommandResponse> obs) {
      pullCount.incrementAndGet();
      PullCommandResponse next = pulls.poll();
      obs.onNext(next != null
          ? next
          : PullCommandResponse.newBuilder().setCommandFound(false).setQueueClosed(true).build());
      obs.onCompleted();
    }

    @Override
    public void pushIntermediateStorage(PushIntermediateStorageRequest request,
                                        StreamObserver<PushIntermediateStorageResponse> obs) {
      pushCount.incrementAndGet();
      if(failPush) {
        obs.onError(Status.INTERNAL.withDescription("engine is unhappy").asRuntimeException());
        return;
      }
      obs.onNext(PushIntermediateStorageResponse.getDefaultInstance());
      obs.onCompleted();
    }

    @Override
    public void reportStatus(StatusReport request, StreamObserver<StatusReportResponse> obs) {
      synchronized(reports) {
        reports.add(request);
      }
      obs.onNext(StatusReportResponse.getDefaultInstance());
      obs.onCompleted();
    }

    @Override
    public void watchInterrupt(WatchInterruptRequest request, StreamObserver<InterruptEvent> obs) {
      obs.onCompleted();
    }

    List<StatusReport.EventType> events() {
      synchronized(reports) {
        return reports.stream().map(StatusReport::getEvent).toList();
      }
    }
  }

  private void run(FakeEngine engine, Indexer indexer) throws IOException {
    String name = InProcessServerBuilder.generateName();
    server = InProcessServerBuilder.forName(name).directExecutor().addService(engine).build().start();
    channel = InProcessChannelBuilder.forName(name).directExecutor().build();
    new GrpcWorker("in-process", 7, indexer).work(channel);
  }

  private static PullCommandResponse command(String path) {
    return PullCommandResponse.newBuilder()
        .setCommandFound(true)
        .setCommand(IndexerCommand.newBuilder()
            .setType(IndexerCommand.CommandType.JAVA)
            .setSourceFilePath(path))
        .build();
  }

  private static final Indexer EmptyStorage = command -> IntermediateStorage.newBuilder().setNextId(1).build();

  @Test
  void an_empty_queue_that_is_not_closed_keeps_the_worker_pulling() throws IOException {
    FakeEngine engine = new FakeEngine();
    engine.pulls.add(PullCommandResponse.newBuilder().setCommandFound(false).setQueueClosed(false).build());
    engine.pulls.add(PullCommandResponse.newBuilder().setCommandFound(false).setQueueClosed(false).build());
    engine.pulls.add(command("/tmp/A.java"));
    // The FakeEngine default answer (queue_closed=true) then ends the run.

    run(engine, EmptyStorage);

    assertEquals(1, engine.pushCount.get(), "the command after the two empty answers must still be indexed");
    assertEquals(4, engine.pullCount.get());
  }

  @Test
  void a_closed_queue_ends_the_run() throws IOException {
    FakeEngine engine = new FakeEngine();
    engine.pulls.add(PullCommandResponse.newBuilder().setCommandFound(false).setQueueClosed(true).build());

    run(engine, EmptyStorage);

    assertEquals(1, engine.pullCount.get());
    assertEquals(0, engine.pushCount.get());
    assertEquals(List.of(StatusReport.EventType.PROCESS_DONE), engine.events());
  }

  @Test
  void a_successful_file_reports_start_then_finish() throws IOException {
    FakeEngine engine = new FakeEngine();
    engine.pulls.add(command("/tmp/A.java"));

    run(engine, EmptyStorage);

    assertEquals(List.of(StatusReport.EventType.START_FILE,
                         StatusReport.EventType.FINISH_FILE,
                         StatusReport.EventType.PROCESS_DONE),
        engine.events());
    assertEquals("/tmp/A.java", engine.reports.get(0).getFilePath());
  }

  @Test
  void a_failed_push_withholds_finish_file_so_the_engine_flags_the_file() throws IOException {
    FakeEngine engine = new FakeEngine();
    engine.failPush = true;
    engine.pulls.add(command("/tmp/A.java"));

    run(engine, EmptyStorage);

    assertFalse(engine.events().contains(StatusReport.EventType.FINISH_FILE),
        "an unpushed file must stay in-flight so it lands in drainAndGetCrashedFiles()");
    assertTrue(engine.events().contains(StatusReport.EventType.PROCESS_DONE));
  }

  @Test
  void process_done_is_always_the_last_report() throws IOException {
    FakeEngine engine = new FakeEngine();
    engine.pulls.add(command("/tmp/A.java"));
    engine.pulls.add(command("/tmp/B.java"));

    run(engine, EmptyStorage);

    List<StatusReport.EventType> events = engine.events();
    assertEquals(StatusReport.EventType.PROCESS_DONE, events.get(events.size() - 1));
    assertEquals(1, events.stream().filter(e -> e == StatusReport.EventType.PROCESS_DONE).count());
  }
}
