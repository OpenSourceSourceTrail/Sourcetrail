#pragma once
#include <ostream>
#include <sstream>
#include <string>

struct CollectOutStream final {
  explicit CollectOutStream(std::ostream& ostream) : mStream{ostream}, mOldBuffer{mStream.rdbuf()} {
    mStream.rdbuf(mBufferStream.rdbuf());
  }

  ~CollectOutStream() {
    close();
  }

  void close() {
    if(mOldBuffer != nullptr) {
      mStream.rdbuf(mOldBuffer);
      mOldBuffer = nullptr;
    }
  }

  std::string str() const {
    return mBufferStream.str();
  }

  CollectOutStream(const CollectOutStream&) = delete;
  CollectOutStream& operator=(const CollectOutStream&) = delete;
  CollectOutStream(CollectOutStream&&) = delete;
  CollectOutStream& operator=(CollectOutStream&&) = delete;

private:
  std::ostream& mStream;
  std::streambuf* mOldBuffer;
  std::stringstream mBufferStream;
};