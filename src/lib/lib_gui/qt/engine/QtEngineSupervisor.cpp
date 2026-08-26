#include "QtEngineSupervisor.h"

#include <algorithm>

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QTimer>

#include "Capabilities.h"
#include "EngineChannel.h"
#include "logging.h"
#include "type/MessageRefreshUI.h"
#include "type/MessageStatus.h"

namespace {

constexpr const char* EngineExecutableName =
#ifdef D_WINDOWS
    "sourcetrail_engine.exe";
#else
    "sourcetrail_engine";
#endif

constexpr std::chrono::milliseconds ShutdownRpcTimeout{2000};
constexpr int ShutdownWaitMs = 1500;
constexpr int KillWaitMs = 1000;

/**
 * The engine's handshake line: "ENGINE_PORT <n> <token>". Returns port 0 if this line is not one.
 *
 * The token is the bearer credential for every later request. It is passed on stdout precisely
 * because only the process that spawned the engine can read it -- a loopback HTTP port is otherwise
 * reachable by anything else running as this user.
 */
struct EngineHandshake {
  int port = 0;
  QString token;
};

EngineHandshake parseEngineHandshake(const QString& line) {
  static const QString prefix = QStringLiteral("ENGINE_PORT ");
  if(!line.startsWith(prefix)) {
    return {};
  }

  const QStringList parts = line.mid(prefix.size()).trimmed().split(QLatin1Char(' '), Qt::SkipEmptyParts);
  if(parts.isEmpty()) {
    return {};
  }

  bool parsed = false;
  EngineHandshake handshake;
  handshake.port = parts.front().toInt(&parsed);
  if(!parsed) {
    return {};
  }
  if(parts.size() > 1) {
    handshake.token = parts.at(1);
  }
  return handshake;
}

void status(const std::wstring& text) {
  MessageStatus(text).dispatch();
}

}    // namespace

std::chrono::milliseconds QtEngineSupervisor::backoffFor(int attempt, std::chrono::milliseconds base) {
  constexpr std::chrono::milliseconds cap{30000};
  // attempt is 1-based; shifting past the cap is pointless, and a large shift is undefined.
  const unsigned int shift = static_cast<unsigned int>(std::clamp(attempt - 1, 0, 16));
  return std::min(cap, base * (1U << shift));
}

QtEngineSupervisor::QtEngineSupervisor(QObject* parent)
    : QObject(parent)
    , mEnginePath(QDir(QCoreApplication::applicationDirPath()).filePath(QString::fromLatin1(EngineExecutableName)))
    // Port 0 is deliberate: the channel exists before the engine does, so callers can hold it from
    // startup. Until the handshake lands, every call simply fails and reports degraded.
    , mChannel(std::make_unique<EngineChannel>("127.0.0.1:0", std::string())) {}

QtEngineSupervisor::~QtEngineSupervisor() {
  stop();
}

void QtEngineSupervisor::setEnginePath(QString path) {
  mEnginePath = std::move(path);
}

void QtEngineSupervisor::setBackoffBase(std::chrono::milliseconds base) {
  mBackoffBase = base;
}

EngineChannel* QtEngineSupervisor::getChannel() const {
  return mChannel.get();
}

void QtEngineSupervisor::start() {
  mStopping = false;
  mAttempt = 0;
  spawn();
}

void QtEngineSupervisor::spawn() {
  if(mStopping) {
    return;
  }

  mStdoutBuffer.clear();
  mProcess = new QProcess(this);
  mProcess->setProcessChannelMode(QProcess::SeparateChannels);
  connect(mProcess, &QProcess::readyReadStandardOutput, this, &QtEngineSupervisor::readStdout);
  connect(mProcess, &QProcess::finished, this, &QtEngineSupervisor::handleExit);
  // Without this a binary that does not exist at all would never report anything.
  connect(mProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if(error == QProcess::FailedToStart) {
      handleExit();
    }
  });

  LOG_INFO("Starting engine: " + mEnginePath.toStdString());
  mProcess->start(mEnginePath, {QStringLiteral("--port"), QStringLiteral("0")});
}

void QtEngineSupervisor::readStdout() {
  // Read even after the handshake: an unread pipe eventually blocks the engine's own logging.
  mStdoutBuffer += QString::fromUtf8(mProcess->readAllStandardOutput());

  qsizetype newline = mStdoutBuffer.indexOf(QLatin1Char('\n'));
  while(newline >= 0) {
    const QString line = mStdoutBuffer.left(newline).trimmed();
    mStdoutBuffer.remove(0, newline + 1);
    newline = mStdoutBuffer.indexOf(QLatin1Char('\n'));

    const EngineHandshake handshake = parseEngineHandshake(line);
    if(handshake.port == 0) {
      continue;
    }

    // Reaching the handshake is the only evidence the engine is healthy, so the backoff resets here
    // rather than on spawn -- otherwise a binary that crashes right after listening restarts forever.
    mAttempt = 0;
    mChannel->reconnect("127.0.0.1:" + std::to_string(handshake.port), handshake.token.toStdString());
    // Also drops the cached capabilities: a restarted engine may have a different plugin set.
    client::Capabilities::instance().setChannel(mChannel.get());
    status(L"Engine connected");
    // The project menu computed its enabled state during QtMainWindow's constructor, when there was
    // no engine and therefore no capabilities; without this New/Edit Project stay dead for the rest
    // of the session. QtMainWindow re-runs updateRefreshActionsEnabled() on this message.
    MessageRefreshUI{}.noStyleReload().dispatch();
    Q_EMIT engineConnected();
  }
}

void QtEngineSupervisor::handleExit() {
  if(mStopping || mProcess == nullptr) {
    return;
  }

  mProcess->deleteLater();
  mProcess = nullptr;
  mChannel->markDegraded();

  ++mAttempt;
  if(mAttempt > MaxRestarts) {
    LOG_ERROR("Engine failed to stay up after " + std::to_string(MaxRestarts) + " restarts; giving up.");
    status(L"Engine unavailable. Open projects can still be browsed with the data already loaded.");
    Q_EMIT engineUnavailable();
    return;
  }

  const auto delay = backoffFor(mAttempt, mBackoffBase);
  LOG_WARNING("Engine exited; restarting in " + std::to_string(delay.count()) + "ms");
  status(L"Engine restarting…");
  QTimer::singleShot(static_cast<int>(delay.count()), this, &QtEngineSupervisor::spawn);
}

void QtEngineSupervisor::stop() {
  mStopping = true;
  if(mProcess == nullptr) {
    return;
  }

  if(mProcess->state() != QProcess::NotRunning) {
    // Ask first: this lets the engine close its storage instead of losing it to a signal.
    std::ignore = mChannel->send("POST", "/api/v1/shutdown", {}, ShutdownRpcTimeout);

    // Then insist, in escalating order: SIGTERM is handled by the engine, SIGKILL by the OS.
    if(!mProcess->waitForFinished(ShutdownWaitMs)) {
      mProcess->terminate();
      if(!mProcess->waitForFinished(ShutdownWaitMs)) {
        LOG_WARNING("Engine did not shut down in time; killing it.");
        mProcess->kill();
        std::ignore = mProcess->waitForFinished(KillWaitMs);
      }
    }
  }

  delete mProcess;
  mProcess = nullptr;
  mChannel->markDegraded();
}
