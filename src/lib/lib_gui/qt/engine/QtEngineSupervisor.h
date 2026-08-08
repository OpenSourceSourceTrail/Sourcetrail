#pragma once
#include <chrono>
#include <memory>

#include <QObject>
#include <QString>

class QProcess;
class EngineChannel;

/**
 * Owns the sourcetrail_engine child process and the channel to it.
 *
 * The engine is spawned with "--port 0" and reports the port it actually got on the first line of
 * its stdout ("ENGINE_PORT <n>"), so several Sourcetrail instances can run side by side. If the
 * engine dies, it is respawned with an exponential backoff; queries made while it is down do not
 * fail loudly, they return empty results (see GrpcStorageAccess), which is what keeps the GUI alive.
 */
class QtEngineSupervisor final : public QObject {
  Q_OBJECT

public:
  /** Stop respawning after this many consecutive failed starts and report the engine unavailable. */
  static constexpr int MaxRestarts = 5;

  static constexpr std::chrono::milliseconds DefaultBackoffBase{1000};

  /** base, 2*base, 4*base, ... capped at 30s, so a permanently broken engine does not spin the CPU. */
  static std::chrono::milliseconds backoffFor(int attempt, std::chrono::milliseconds base = DefaultBackoffBase);

  /** Default engine path: sourcetrail_engine next to the running GUI binary. */
  explicit QtEngineSupervisor(QObject* parent = nullptr);
  ~QtEngineSupervisor() override;

  Q_DISABLE_COPY_MOVE(QtEngineSupervisor)

  /** Overrides the engine binary; only useful for tests. Call before start(). */
  void setEnginePath(QString path);

  /** Shortens the restart backoff; only useful for tests. */
  void setBackoffBase(std::chrono::milliseconds base);

  void start();

  /** Asks the engine to shut down, and kills it if it does not. Idempotent; also called on destruction. */
  void stop();

  /**
   * Never null and stable for the supervisor's lifetime, so it can be handed out before the engine
   * has even been spawned -- it simply reports degraded until the first handshake.
   */
  [[nodiscard]] EngineChannel* getChannel() const;

Q_SIGNALS:
  /** The engine is up and the channel points at it. Emitted on the first start and every restart. */
  void engineConnected();

  /** Gave up after MaxRestarts; the GUI stays usable but read-only against nothing. */
  void engineUnavailable();

private:
  void spawn();
  void readStdout();
  void handleExit();

  QString mEnginePath;
  QProcess* mProcess = nullptr;
  std::unique_ptr<EngineChannel> mChannel;
  QString mStdoutBuffer;
  std::chrono::milliseconds mBackoffBase = DefaultBackoffBase;
  int mAttempt = 0;
  bool mStopping = false;
};
