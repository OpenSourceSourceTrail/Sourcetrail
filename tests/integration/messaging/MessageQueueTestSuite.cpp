#include <chrono>
#include <memory>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#define private public
#include "MessageQueue.h"
#undef private
#include "../../../src/scheduling/TaskScheduler.h"
#include "Message.h"
#include "MessageFilter.h"
#include "MessageListener.h"
#include "TabId.h"
#include "ITaskManager.hpp"

#include "mocks/MockedTaskManager.hpp"


namespace {
class TestMessage : public Message<TestMessage> {
public:
  static std::string getStaticType() {
    return "TestMessage";
  }
};

class Test2Message : public Message<Test2Message> {
public:
  static std::string getStaticType() {
    return "TestMessage2";
  }
};

class TestMessageListener : public MessageListener<TestMessage> {
public:
  TestMessageListener() = default;

  int m_messageCount = 0;

private:
  void handleMessage(TestMessage* /*message*/) override {
    ++m_messageCount;
  }
};

class Test2MessageListener : public MessageListener<Test2Message> {
public:
  Test2MessageListener() = default;

  int m_messageCount = 0;

private:
  void handleMessage(Test2Message* /*message*/) override {
    m_messageCount++;
    TestMessage().dispatch();
  }
};

class Test3MessageListener : public MessageListener<Test2Message> {
public:
  std::shared_ptr<TestMessageListener> m_listener;

private:
  void handleMessage(Test2Message* /*message*/) override {
    m_listener = std::make_shared<TestMessageListener>();
  }
};

class Test4MessageListener
    : public MessageListener<TestMessage>
    , public MessageListener<Test2Message> {
public:
  std::shared_ptr<TestMessageListener> m_listener;

private:
  void handleMessage(TestMessage* /*message*/) override {
    if(!m_listener) {
      m_listener = std::make_shared<TestMessageListener>();
    }
  }

  void handleMessage(Test2Message* /*message*/) override {
    m_listener.reset();
  }
};

class Test5MessageListener : public MessageListener<TestMessage> {
public:
  std::vector<std::shared_ptr<TestMessageListener>> m_listeners;

private:
  void handleMessage(TestMessage* /*message*/) override {
    if(m_listeners.empty()) {
      for(size_t i = 0; i < 5; i++) {
        m_listeners.push_back(std::make_shared<TestMessageListener>());
      }
    }
  }
};

void waitForThread() {
  static const auto THREAD_WAIT_TIME_MS = std::chrono::milliseconds(25);
  do {
    std::this_thread::sleep_for(THREAD_WAIT_TIME_MS);
  } while(IMessageQueue::getInstance()->hasMessagesQueued());
}
}    // namespace

TEST(MessageQueue, singleInstance) {
#if !defined(NDEBUG)
  EXPECT_DEATH(IMessageQueue::getInstance(), "");
  EXPECT_DEATH(IMessageQueue::getInstanceRaw(), "");
#endif

  IMessageQueue::setInstance(std::make_shared<details::MessageQueue>());
  auto messageQueue0 = IMessageQueue::getInstance();
  auto messageQueue1 = IMessageQueue::getInstance();
  EXPECT_EQ(messageQueue0, messageQueue1);
  IMessageQueue::setInstance(nullptr);
}

struct MessageQueueFix : testing::Test {
  void SetUp() override {
    IMessageQueue::setInstance(std::make_shared<details::MessageQueue>());
  }
  void TearDown() override {
    IMessageQueue::setInstance(nullptr);
  }
};

TEST_F(MessageQueueFix, messageLoopStartsAndStops) {
  auto* messageQueue = IMessageQueue::getInstanceRaw();
  EXPECT_FALSE(messageQueue->loopIsRunning());

  messageQueue->startMessageLoopThreaded();

  waitForThread();

  EXPECT_TRUE(messageQueue->loopIsRunning());

  messageQueue->stopMessageLoop();

  waitForThread();

  EXPECT_FALSE(messageQueue->loopIsRunning());
}

TEST_F(MessageQueueFix, registeredListenerReceivesMessages) {
  auto* messageQueue = IMessageQueue::getInstanceRaw();
  messageQueue->startMessageLoopThreaded();

  TestMessageListener listener;
  Test2MessageListener listener2;

  TestMessage().dispatch();
  TestMessage().dispatch();
  TestMessage().dispatch();

  waitForThread();

  messageQueue->stopMessageLoop();

  EXPECT_EQ(3, listener.m_messageCount);
  EXPECT_EQ(0, listener2.m_messageCount);
}

TEST_F(MessageQueueFix, messageDispatchingWithinMessageHandling) {
  auto* messageQueue = IMessageQueue::getInstanceRaw();
  messageQueue->startMessageLoopThreaded();

  TestMessageListener listener;
  Test2MessageListener listener2;

  Test2Message().dispatch();

  waitForThread();

  messageQueue->stopMessageLoop();

  EXPECT_EQ(1, listener.m_messageCount);
  EXPECT_EQ(1, listener2.m_messageCount);
}

TEST_F(MessageQueueFix, listenerRegistrationWithinMessageHandling) {
  auto* messageQueue = IMessageQueue::getInstanceRaw();
  messageQueue->startMessageLoopThreaded();

  Test3MessageListener listener;

  Test2Message().dispatch();
  TestMessage().dispatch();

  waitForThread();

  messageQueue->stopMessageLoop();

  ASSERT_TRUE(listener.m_listener);
  EXPECT_EQ(1, listener.m_listener->m_messageCount);
}

TEST_F(MessageQueueFix, listenerUnregistrationWithinMessageHandling) {
  auto* messageQueue = IMessageQueue::getInstanceRaw();
  messageQueue->startMessageLoopThreaded();

  Test4MessageListener listener;

  TestMessage().dispatch();

  Test2Message().dispatch();

  TestMessage().dispatch();
  TestMessage().dispatch();
  TestMessage().dispatch();

  waitForThread();

  messageQueue->stopMessageLoop();

  ASSERT_TRUE(listener.m_listener);
  EXPECT_EQ(2, listener.m_listener->m_messageCount);
}

TEST_F(MessageQueueFix, listenerRegistrationToFrontAndBackWithinMessageHandling) {
  auto* messageQueue = IMessageQueue::getInstanceRaw();
  messageQueue->startMessageLoopThreaded();

  Test5MessageListener listener;

  TestMessage().dispatch();
  TestMessage().dispatch();
  TestMessage().dispatch();

  waitForThread();

  messageQueue->stopMessageLoop();

  ASSERT_EQ(5, listener.m_listeners.size());
  EXPECT_EQ(2, listener.m_listeners[0]->m_messageCount);
  EXPECT_EQ(2, listener.m_listeners[1]->m_messageCount);
  EXPECT_EQ(2, listener.m_listeners[2]->m_messageCount);
  EXPECT_EQ(2, listener.m_listeners[3]->m_messageCount);
  EXPECT_EQ(2, listener.m_listeners[4]->m_messageCount);
}

struct MessageQueueRegistration : testing::Test {
  void SetUp() override {
    auto mQueue = std::make_shared<details::MessageQueue>();
    messageQueue = mQueue.get();
    IMessageQueue::setInstance(std::move(mQueue));

    messageQueue->mListeners.clear();
  }

  void TearDown() override {
    messageQueue->mListeners.clear();
    if(messageQueue->loopIsRunning()) {
      messageQueue->stopMessageLoop();
    }
    IMessageQueue::setInstance(nullptr);
  }

  details::MessageQueue* messageQueue = nullptr;
};

TEST_F(MessageQueueRegistration, registerIsUnique) {
  TestMessageListener messageListener;

  messageQueue->registerListener(&messageListener);
  messageQueue->registerListener(&messageListener);
  messageQueue->registerListener(&messageListener);

  EXPECT_EQ(1, messageQueue->mListeners.size());
}

TEST_F(MessageQueueRegistration, registerGoodCase) {
  TestMessageListener messageListener;
  ASSERT_EQ(1, messageQueue->mListeners.size());

  messageQueue->startMessageLoopThreaded();

  TestMessage {}.dispatch();

  waitForThread();
  messageQueue->stopMessageLoop();

  EXPECT_EQ(1, messageListener.m_messageCount);

  messageQueue->unregisterListener(&messageListener);
  EXPECT_THAT(messageQueue->mListeners, testing::IsEmpty());

  TestMessage {}.dispatch();

  EXPECT_EQ(1, messageListener.m_messageCount);
}

struct MessageQueueProcess : testing::Test {
  void SetUp() override {
    auto mQueue = std::make_shared<details::MessageQueue>();
    messageQueue = mQueue.get();
    IMessageQueue::setInstance(std::move(mQueue));

    messageQueue->mMessageBuffer.clear();
    messageQueue->mFilters.clear();

    mMockedTaskManager = std::make_shared<scheduling::mocks::MockedTaskManager>();
    scheduling::ITaskManager::setInstance(mMockedTaskManager);
  }

  void TearDown() override {
    messageQueue->setSendMessagesAsTasks(false);
    messageQueue->mFilters.clear();
    messageQueue->mMessageBuffer.clear();
    IMessageQueue::setInstance(nullptr);

    scheduling::ITaskManager::setInstance(nullptr);
    mMockedTaskManager.reset();
  }

  details::MessageQueue* messageQueue = nullptr;
  std::shared_ptr<scheduling::mocks::MockedTaskManager> mMockedTaskManager;
};

TEST_F(MessageQueueProcess, goodCase) {
  TestMessageListener messageListener;
  ASSERT_EQ(1, messageQueue->mListeners.size());

  auto message = std::make_shared<TestMessage>();
  message->setIsLogged(true);

  messageQueue->processMessage(message, false);

  EXPECT_EQ(1, messageListener.m_messageCount);
}


TEST_F(MessageQueueProcess, goodCaseAsTask) {
  auto fakeTask = std::make_shared<TaskScheduler>(Id{});
  EXPECT_CALL(*mMockedTaskManager, getScheduler).WillOnce(testing::Return(fakeTask));

  messageQueue->setSendMessagesAsTasks(true);

  TestMessageListener messageListener;
  ASSERT_EQ(1, messageQueue->mListeners.size());

  auto message = std::make_shared<TestMessage>();
  message->setSendAsTask(true);
  message->setIsLogged(true);

  messageQueue->processMessage(message, true);

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(50ms);

  EXPECT_EQ(0, messageListener.m_messageCount);
}

struct MessageQueueSendMessage : testing::Test {
  void SetUp() override {
    auto mQueue = std::make_shared<details::MessageQueue>();
    messageQueue = mQueue.get();
    IMessageQueue::setInstance(std::move(mQueue));
  }

  void TearDown() override {
    IMessageQueue::setInstance(nullptr);
  }

  details::MessageQueue* messageQueue = nullptr;
};

TEST_F(MessageQueueSendMessage, goodCase) {
  TestMessageListener messageListener;
  ASSERT_EQ(1, messageQueue->mListeners.size());

  auto message = std::make_shared<TestMessage>();
  message->setIsLogged(true);

  messageQueue->sendMessage(message);
  EXPECT_EQ(1, messageListener.m_messageCount);
}

struct MockedMessageFilter : MessageFilter {
  MOCK_METHOD(void, filter, (IMessageQueue::MessageBufferType*), (override));
};

struct MessageQueueFilter : testing::Test {
  void SetUp() override {
    auto mQueue = std::make_shared<details::MessageQueue>();
    messageQueue = mQueue.get();
    IMessageQueue::setInstance(std::move(mQueue));
  }

  void TearDown() override {
    IMessageQueue::setInstance(nullptr);
  }

  details::MessageQueue* messageQueue = nullptr;
};

TEST_F(MessageQueueFilter, goodCase) {
  auto filter = std::make_shared<testing::StrictMock<MockedMessageFilter>>();

  messageQueue->addMessageFilter(filter);
  messageQueue->addMessageFilter(filter);
  messageQueue->addMessageFilter(filter);

  EXPECT_EQ(1, messageQueue->mFilters.size());
}