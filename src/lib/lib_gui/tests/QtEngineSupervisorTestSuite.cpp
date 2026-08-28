#include <array>
#include <chrono>
#include <functional>
#include <memory>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include "EngineChannel.h"
#include "MessageQueue.h"
#include "QtEngineSupervisor.h"

using namespace std::chrono_literals;

namespace {

/** QProcess and QTimer both need a running application object; gtest_main does not create one. */
void ensureApplication() {
  if(QCoreApplication::instance() != nullptr) {
    return;
  }
  static int argc = 1;
  static char arg0[] = "QtEngineSupervisorTestSuite";
  static std::array<char*, 2> argv{arg0, nullptr};
  static QCoreApplication app(argc, argv.data());
}

/**
 * The supervisor's whole job is reacting to a child process, so these tests drive it with real
 * child processes -- stub "engines" written as shell scripts. Mocking QProcess would test nothing.
 */
class QtEngineSupervisorFix : public testing::Test {
protected:
  void SetUp() override {
#ifdef D_WINDOWS
    GTEST_SKIP() << "The stub engines are POSIX shell scripts.";
#endif
    ensureApplication();
    // The supervisor reports its state through MessageStatus, which needs a queue to push into.
    IMessageQueue::setInstance(std::make_shared<details::MessageQueue>());
    ASSERT_TRUE(mDir.isValid());
    // Constructed only after the application object exists: a QProcess without an event dispatcher
    // has nothing to deliver its signals.
    mSupervisor = std::make_unique<QtEngineSupervisor>();
    mSupervisor->setBackoffBase(20ms);
  }

  /** Writes an executable stub engine and points the supervisor at it. */
  void useStubEngine(const QString& body) {
    const QString path = QDir(mDir.path()).filePath(QStringLiteral("stub_engine.sh"));
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_GT(file.write(("#!/bin/sh\n" + body).toUtf8()), 0);
    file.close();
    ASSERT_TRUE(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    mSupervisor->setEnginePath(path);
  }

  /** Spins the event loop until `predicate` holds or the timeout expires. */
  static bool waitFor(const std::function<bool()>& predicate, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while(std::chrono::steady_clock::now() < deadline) {
      if(predicate()) {
        return true;
      }
      QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
    return predicate();
  }

  void TearDown() override {
    mSupervisor.reset();
    IMessageQueue::setInstance(nullptr);
  }

  QTemporaryDir mDir;
  std::unique_ptr<QtEngineSupervisor> mSupervisor;
};

}    // namespace

TEST(QtEngineSupervisorBackoff, doublesPerAttemptAndCaps) {
  EXPECT_EQ(QtEngineSupervisor::backoffFor(1), 1000ms);
  EXPECT_EQ(QtEngineSupervisor::backoffFor(2), 2000ms);
  EXPECT_EQ(QtEngineSupervisor::backoffFor(3), 4000ms);
  EXPECT_EQ(QtEngineSupervisor::backoffFor(5), 16000ms);
  // Capped, and a nonsensical attempt must not shift into undefined behaviour.
  EXPECT_EQ(QtEngineSupervisor::backoffFor(6), 30000ms);
  EXPECT_EQ(QtEngineSupervisor::backoffFor(99), 30000ms);
  EXPECT_EQ(QtEngineSupervisor::backoffFor(0), 1000ms);
}

TEST_F(QtEngineSupervisorFix, channelExistsBeforeTheEngineDoes) {
  // Callers get the channel at construction time and hold it across restarts.
  ASSERT_NE(mSupervisor->getChannel(), nullptr);
  EXPECT_FALSE(mSupervisor->getChannel()->isConnected());
}

TEST_F(QtEngineSupervisorFix, adoptsThePortFromTheHandshakeLine) {
  useStubEngine(QStringLiteral("echo ENGINE_PORT 45678\nsleep 30\n"));
  QSignalSpy connected(mSupervisor.get(), &QtEngineSupervisor::engineConnected);

  mSupervisor->start();

  ASSERT_TRUE(waitFor([&] { return !connected.isEmpty(); }, 5s));
  EXPECT_EQ(mSupervisor->getChannel()->getEndpoint(), "127.0.0.1:45678");
}

TEST_F(QtEngineSupervisorFix, ignoresNonHandshakeOutput) {
  useStubEngine(QStringLiteral("echo starting up\necho ENGINE_PORT 45679\necho listening\nsleep 30\n"));
  QSignalSpy connected(mSupervisor.get(), &QtEngineSupervisor::engineConnected);

  mSupervisor->start();

  ASSERT_TRUE(waitFor([&] { return !connected.isEmpty(); }, 5s));
  EXPECT_EQ(connected.size(), 1);
  EXPECT_EQ(mSupervisor->getChannel()->getEndpoint(), "127.0.0.1:45679");
}

TEST_F(QtEngineSupervisorFix, restartsAnEngineThatDies) {
  const QString marker = QDir(mDir.path()).filePath(QStringLiteral("runs"));
  useStubEngine(QStringLiteral("echo x >> %1\necho ENGINE_PORT 45680\n").arg(marker));
  QSignalSpy connected(mSupervisor.get(), &QtEngineSupervisor::engineConnected);

  mSupervisor->start();

  // Each run announces a port and exits at once, so a working restart loop keeps reconnecting.
  EXPECT_TRUE(waitFor([&] { return connected.size() >= 3; }, 5s));
}

TEST_F(QtEngineSupervisorFix, givesUpAfterRepeatedImmediateFailures) {
  useStubEngine(QStringLiteral("exit 1\n"));
  QSignalSpy unavailable(mSupervisor.get(), &QtEngineSupervisor::engineUnavailable);

  mSupervisor->start();

  ASSERT_TRUE(waitFor([&] { return !unavailable.isEmpty(); }, 10s));
  EXPECT_FALSE(mSupervisor->getChannel()->isConnected());

  // And it must stay given up rather than quietly resuming the loop.
  EXPECT_FALSE(waitFor([&] { return unavailable.size() > 1; }, 500ms));
}

TEST_F(QtEngineSupervisorFix, reportsUnavailableWhenTheEngineBinaryIsMissing) {
  mSupervisor->setEnginePath(QDir(mDir.path()).filePath(QStringLiteral("does_not_exist")));
  QSignalSpy unavailable(mSupervisor.get(), &QtEngineSupervisor::engineUnavailable);

  mSupervisor->start();

  EXPECT_TRUE(waitFor([&] { return !unavailable.isEmpty(); }, 10s));
}

TEST_F(QtEngineSupervisorFix, stopEndsTheProcessAndSuppressesRestarts) {
  useStubEngine(QStringLiteral("echo ENGINE_PORT 45681\nsleep 30\n"));
  QSignalSpy connected(mSupervisor.get(), &QtEngineSupervisor::engineConnected);
  ASSERT_TRUE((mSupervisor->start(), waitFor([&] { return !connected.isEmpty(); }, 5s)));

  // The stub answers no Shutdown RPC, so this also covers the kill-on-timeout path.
  mSupervisor->stop();

  EXPECT_FALSE(mSupervisor->getChannel()->isConnected());
  EXPECT_FALSE(waitFor([&] { return connected.size() > 1; }, 500ms));
}
