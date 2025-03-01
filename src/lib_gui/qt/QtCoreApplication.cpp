#include "QtCoreApplication.h"
// STL
#include <iostream>

QtCoreApplication::QtCoreApplication(int argc, char** argv) : QCoreApplication(argc, argv) {}

QtCoreApplication::~QtCoreApplication() = default;

void QtCoreApplication::handleMessage([[maybe_unused]] MessageQuitApplication* message) {
  std::cout << "Quitting" << std::endl;
  emit quit();
}

void QtCoreApplication::handleMessage(MessageIndexingStatus* message) {
  if(message->showProgress) {
    std::cout << message->progressPercent << "% " << '\r' << std::flush;
  }
}

void QtCoreApplication::handleMessage(MessageStatus* message) {
  if(message->isError) {
    std::wcout << L"ERROR: ";
  }

  for(const std::wstring& status : message->stati()) {
    std::wcout << status << std::endl;
  }
}
