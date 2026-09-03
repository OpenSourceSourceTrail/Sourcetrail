#include "HttpClient.h"

#include <mutex>

#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <string_view>
#include <sys/socket.h>

#include "logging.h"

namespace beast = boost::beast;
namespace asio = boost::asio;
namespace bhttp = boost::beast::http;
using tcp = boost::asio::ip::tcp;

namespace http {

struct Client::Impl {
  Impl(std::string hostName, uint16_t portNumber, std::string token)
      : host(std::move(hostName)), port(portNumber), authToken(std::move(token)), socket(io) {}

  std::string host;
  uint16_t port;
  std::string authToken;

  asio::io_context io;
  tcp::socket socket;
  bool connected{false};

  void disconnect() {
    if(connected) {
      boost::system::error_code ignored;
      socket.shutdown(tcp::socket::shutdown_both, ignored);
      socket.close(ignored);
      connected = false;
    }
  }

  /**
   * Applies `timeout` to the socket itself.
   *
   * Beast's tcp_stream::expires_after only governs *asynchronous* operations, and these reads and
   * writes are synchronous -- so the deadline has to be a socket option or it does not exist. A
   * caller blocked forever on an engine that accepted the connection and then wedged is exactly the
   * failure this boundary must survive.
   */
  void applyTimeout(std::chrono::milliseconds timeout) {
    timeval value{};
    value.tv_sec = static_cast<decltype(value.tv_sec)>(timeout.count() / 1000);
    value.tv_usec = static_cast<decltype(value.tv_usec)>((timeout.count() % 1000) * 1000);
    const int handle = static_cast<int>(socket.native_handle());
    ::setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, &value, sizeof(value));
    ::setsockopt(handle, SOL_SOCKET, SO_SNDTIMEO, &value, sizeof(value));
  }

  bool connect(std::chrono::milliseconds timeout) {
    if(connected) {
      return true;
    }

    boost::system::error_code errorCode;
    const tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), port);
    socket.open(endpoint.protocol(), errorCode);
    if(errorCode) {
      return false;
    }
    // Loopback connects either succeed or are refused immediately, so no separate connect deadline.
    socket.connect(endpoint, errorCode);
    if(errorCode) {
      socket.close(errorCode);
      return false;
    }
    connected = true;
    applyTimeout(timeout);
    return true;
  }

  /** One attempt on the current connection. `retryable` says a fresh connection is worth trying. */
  std::optional<ClientResponse> attempt(const std::string& method,
                                        const std::string& target,
                                        const std::string& body,
                                        std::chrono::milliseconds timeout,
                                        bool& retryable) {
    retryable = false;
    if(!connect(timeout)) {
      return std::nullopt;
    }

    bhttp::request<bhttp::string_body> req{bhttp::string_to_verb(method), target, 11};
    req.set(bhttp::field::host, host);
    req.set(bhttp::field::authorization, "Bearer " + authToken);
    req.set(bhttp::field::content_type, "application/json");
    req.keep_alive(true);
    req.body() = body;
    req.prepare_payload();

    boost::system::error_code errorCode;
    applyTimeout(timeout);
    bhttp::write(socket, req, errorCode);
    if(errorCode) {
      // A keep-alive socket the server has since closed fails here on the first write.
      retryable = connected;
      disconnect();
      return std::nullopt;
    }

    beast::flat_buffer buffer;
    bhttp::response<bhttp::string_body> res;
    bhttp::read(socket, buffer, res, errorCode);
    if(errorCode) {
      retryable = connected;
      disconnect();
      return std::nullopt;
    }

    ClientResponse response;
    response.status = static_cast<int>(res.result_int());
    response.body = std::move(res.body());

    if(!res.keep_alive()) {
      disconnect();
    }
    return response;
  }
};

Client::Client(std::string host, uint16_t port, std::string authToken)
    : mImpl(std::make_unique<Impl>(std::move(host), port, std::move(authToken))) {}

Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;

Client::~Client() {
  if(mImpl) {
    mImpl->disconnect();
  }
}

void Client::reconnect(std::string host, uint16_t port, std::string authToken) {
  mImpl->disconnect();
  mImpl->host = std::move(host);
  mImpl->port = port;
  mImpl->authToken = std::move(authToken);
}

std::optional<ClientResponse> Client::request(const std::string& method,
                                              const std::string& target,
                                              const std::string& body,
                                              std::chrono::milliseconds timeout) {
  bool retryable = false;
  if(auto response = mImpl->attempt(method, target, body, timeout, retryable); response.has_value()) {
    return response;
  }

  // Retried once and only when a previously good socket died mid-request -- otherwise a genuinely
  // down engine would cost every caller two connect timeouts instead of one.
  if(retryable) {
    bool ignored = false;
    return mImpl->attempt(method, target, body, timeout, ignored);
  }
  return std::nullopt;
}

// ---- EventSource -----------------------------------------------------------

struct EventSource::Impl {
  Impl(std::string hostName, uint16_t portNumber, std::string token)
      : host(std::move(hostName)), port(portNumber), authToken(std::move(token)), socket(io) {}

  std::string host;
  uint16_t port;
  std::string authToken;

  asio::io_context io;
  std::mutex socketMutex;
  tcp::socket socket;
  bool cancelled{false};
};

EventSource::EventSource(std::string host, uint16_t port, std::string authToken)
    : mImpl(std::make_unique<Impl>(std::move(host), port, std::move(authToken))) {}

EventSource::~EventSource() {
  cancel();
}

void EventSource::reconnect(std::string host, uint16_t port, std::string authToken) {
  const std::lock_guard<std::mutex> lock(mImpl->socketMutex);
  mImpl->host = std::move(host);
  mImpl->port = port;
  mImpl->authToken = std::move(authToken);
  mImpl->cancelled = false;
}

void EventSource::cancel() {
  const std::lock_guard<std::mutex> lock(mImpl->socketMutex);
  mImpl->cancelled = true;
  boost::system::error_code ignored;
  mImpl->socket.close(ignored);
}

bool EventSource::run(const std::string& target, const EventHandler& onEvent) {
  boost::system::error_code errorCode;
  const tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), mImpl->port);

  {
    const std::lock_guard<std::mutex> lock(mImpl->socketMutex);
    if(mImpl->cancelled) {
      return false;
    }
    mImpl->socket = tcp::socket(mImpl->io);
    mImpl->socket.open(endpoint.protocol(), errorCode);
    if(errorCode) {
      return false;
    }
  }

  mImpl->socket.connect(endpoint, errorCode);
  if(errorCode) {
    return false;
  }

  const std::string request = "GET " + target + " HTTP/1.1\r\nHost: " + mImpl->host + "\r\nAuthorization: Bearer " +
      mImpl->authToken + "\r\nAccept: text/event-stream\r\nCache-Control: no-cache\r\n\r\n";
  asio::write(mImpl->socket, asio::buffer(request), errorCode);
  if(errorCode) {
    return false;
  }

  asio::streambuf buffer;
  // Consume the response head; a non-200 (a rejected token, say) has no stream to read.
  const size_t headSize = asio::read_until(mImpl->socket, buffer, "\r\n\r\n", errorCode);
  if(errorCode) {
    return false;
  }
  {
    const std::string head(asio::buffers_begin(buffer.data()), asio::buffers_begin(buffer.data()) + static_cast<long>(headSize));
    if(head.find(" 200 ") == std::string::npos) {
      LOG_WARNING("Engine refused the event stream: " + head.substr(0, head.find('\r')));
      return false;
    }
    buffer.consume(headSize);
  }

  // Frames are separated by a blank line; ":" lines are keep-alive comments and carry no event.
  while(true) {
    const size_t frameSize = asio::read_until(mImpl->socket, buffer, "\n\n", errorCode);
    if(errorCode) {
      return true;
    }

    const std::string frame(asio::buffers_begin(buffer.data()), asio::buffers_begin(buffer.data()) + static_cast<long>(frameSize));
    buffer.consume(frameSize);

    std::string name;
    std::string data;
    size_t lineStart = 0;
    while(lineStart < frame.size()) {
      size_t lineEnd = frame.find('\n', lineStart);
      if(lineEnd == std::string::npos) {
        lineEnd = frame.size();
      }
      const std::string_view line(frame.data() + lineStart, lineEnd - lineStart);
      if(line.rfind("event: ", 0) == 0) {
        name = std::string(line.substr(7));
      } else if(line.rfind("data: ", 0) == 0) {
        // Multiple data lines concatenate, per the SSE grammar.
        if(!data.empty()) {
          data.push_back('\n');
        }
        data += line.substr(6);
      }
      lineStart = lineEnd + 1;
    }

    if(!name.empty() && onEvent) {
      onEvent(name, data);
    }
  }
}

}    // namespace http
