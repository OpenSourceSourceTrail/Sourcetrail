#include "ConsoleApplication.h"

#include <iostream>

void ConsoleApplication::run() {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_condition.wait(lock, [this] { return m_quit; });
}

void ConsoleApplication::handleMessage([[maybe_unused]] MessageQuitApplication* message) {
  std::cout << "Quitting" << std::endl;
  {
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_quit = true;
  }
  m_condition.notify_all();
}

void ConsoleApplication::handleMessage(MessageIndexingStatus* message) {
  if(message->showProgress) {
    std::cout << message->progressPercent << "% " << '\r' << std::flush;
  }
}

void ConsoleApplication::handleMessage(MessageStatus* message) {
  if(message->isError) {
    std::wcout << L"ERROR: ";
  }

  for(const std::wstring& status : message->stati()) {
    std::wcout << status << std::endl;
  }
}
