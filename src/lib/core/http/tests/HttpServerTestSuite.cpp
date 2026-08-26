#include <chrono>
#include <string>
#include <thread>

#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

#include <gtest/gtest.h>

#include "HttpClient.h"
#include "HttpServer.h"

using namespace std::chrono_literals;

namespace {

constexpr auto Timeout = 2000ms;
const std::string Token = "test-token";

/** A server with the routes every case below shares, already listening on an ephemeral port. */
struct Fixture {
  http::Server server{Token};
  uint16_t port{0};

  Fixture() {
    server.route("GET", "/api/v1/stats", [](const http::Request&) { return http::Response::json(R"({"nodes":7})"); });

    server.route("GET", "/api/v1/search", [](const http::Request& request) {
      return http::Response::json(R"({"q":")" + request.get("q") + R"(","limit":)" +
                                  std::to_string(request.getUInt("limit", 10)) + "}");
    });

    // Trailing slash makes this a prefix route: everything after it lands in Request::param.
    server.route("GET", "/api/v1/files/", [](const http::Request& request) {
      return http::Response::json(R"({"path":")" + request.param + R"("})");
    });

    server.route("POST", "/api/v1/echo", [](const http::Request& request) { return http::Response::json(request.body); });

    server.route(
        "GET", "/api/v1/boom", [](const http::Request&) -> http::Response { throw std::runtime_error("handler blew up"); });

    server.eventStream("/api/v1/events");
    port = server.start(0);
  }

  http::Client client() {
    return http::Client{"127.0.0.1", port, Token};
  }
};

}    // namespace

TEST(HttpServer, bindsAnEphemeralPort) {
  const Fixture fixture;
  EXPECT_NE(fixture.port, 0);
}

TEST(HttpServer, servesAnExactRoute) {
  Fixture fixture;
  auto client = fixture.client();

  const auto response = client.request("GET", "/api/v1/stats", {}, Timeout);
  ASSERT_TRUE(response.has_value());
  EXPECT_EQ(response->status, 200);
  EXPECT_EQ(response->body, R"({"nodes":7})");
}

TEST(HttpServer, parsesAndDecodesQueryParameters) {
  Fixture fixture;
  auto client = fixture.client();

  // %3A is ':' and '+' is a space -- both must survive decoding.
  const auto response = client.request("GET", "/api/v1/search?q=foo%3A+bar&limit=25", {}, Timeout);
  ASSERT_TRUE(response.has_value());
  EXPECT_EQ(response->body, R"({"q":"foo: bar","limit":25})");
}

TEST(HttpServer, fallsBackWhenAQueryParameterIsAbsent) {
  Fixture fixture;
  auto client = fixture.client();

  const auto response = client.request("GET", "/api/v1/search?q=x", {}, Timeout);
  ASSERT_TRUE(response.has_value());
  EXPECT_EQ(response->body, R"({"q":"x","limit":10})");
}

TEST(HttpServer, prefixRouteCapturesTheRemainder) {
  Fixture fixture;
  auto client = fixture.client();

  const auto response = client.request("GET", "/api/v1/files/src%2Fmain.cpp", {}, Timeout);
  ASSERT_TRUE(response.has_value());
  EXPECT_EQ(response->body, R"({"path":"src/main.cpp"})");
}

TEST(HttpServer, carriesARequestBody) {
  Fixture fixture;
  auto client = fixture.client();

  const auto response = client.request("POST", "/api/v1/echo", R"({"ids":[1,2,3]})", Timeout);
  ASSERT_TRUE(response.has_value());
  EXPECT_EQ(response->body, R"({"ids":[1,2,3]})");
}

TEST(HttpServer, reusesOneKeepAliveConnectionAcrossCalls) {
  Fixture fixture;
  auto client = fixture.client();

  for(int i = 0; i < 5; ++i) {
    const auto response = client.request("GET", "/api/v1/stats", {}, Timeout);
    ASSERT_TRUE(response.has_value()) << "call " << i;
    EXPECT_EQ(response->status, 200);
  }
}

TEST(HttpServer, answers404ForAnUnknownPath) {
  Fixture fixture;
  auto client = fixture.client();

  const auto response = client.request("GET", "/api/v1/nope", {}, Timeout);
  ASSERT_TRUE(response.has_value());
  EXPECT_EQ(response->status, 404);
}

TEST(HttpServer, answers500WhenAHandlerThrows) {
  Fixture fixture;
  auto client = fixture.client();

  const auto response = client.request("GET", "/api/v1/boom", {}, Timeout);
  ASSERT_TRUE(response.has_value());
  EXPECT_EQ(response->status, 500);
}

TEST(HttpServer, rejectsAMissingOrWrongBearerToken) {
  Fixture fixture;

  http::Client anonymous{"127.0.0.1", fixture.port, ""};
  const auto missing = anonymous.request("GET", "/api/v1/stats", {}, Timeout);
  ASSERT_TRUE(missing.has_value());
  EXPECT_EQ(missing->status, 401);

  http::Client wrong{"127.0.0.1", fixture.port, "not-the-token"};
  const auto bad = wrong.request("GET", "/api/v1/stats", {}, Timeout);
  ASSERT_TRUE(bad.has_value());
  EXPECT_EQ(bad->status, 401);
}

TEST(HttpServer, rejectsAnUnknownOrigin) {
  namespace asio = boost::asio;
  using tcp = boost::asio::ip::tcp;

  Fixture fixture;
  fixture.server.allowOrigin("http://localhost:3000");

  const auto statusFor = [&](const std::string& origin) {
    asio::io_context io;
    tcp::socket socket(io);
    socket.connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"), fixture.port));

    const std::string request = "GET /api/v1/stats HTTP/1.1\r\nHost: 127.0.0.1\r\nAuthorization: Bearer " + Token +
        "\r\nOrigin: " + origin + "\r\nConnection: close\r\n\r\n";
    asio::write(socket, asio::buffer(request));

    asio::streambuf buffer;
    boost::system::error_code errorCode;
    asio::read_until(socket, buffer, "\r\n", errorCode);
    std::istream stream(&buffer);
    std::string version;
    int status = 0;
    stream >> version >> status;
    return status;
  };

  EXPECT_EQ(statusFor("http://evil.example"), 403);
  EXPECT_EQ(statusFor("http://localhost:3000"), 200);
}

TEST(HttpServer, broadcastReachesAnOpenEventStream) {
  namespace asio = boost::asio;
  using tcp = boost::asio::ip::tcp;

  Fixture fixture;

  asio::io_context io;
  tcp::socket socket(io);
  socket.connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"), fixture.port));

  const std::string request = "GET /api/v1/events HTTP/1.1\r\nHost: 127.0.0.1\r\nAuthorization: Bearer " + Token +
      "\r\nAccept: text/event-stream\r\n\r\n";
  asio::write(socket, asio::buffer(request));

  asio::streambuf buffer;
  // Read past the response headers before broadcasting, so the stream is registered.
  asio::read_until(socket, buffer, "\r\n\r\n");

  fixture.server.broadcast("indexingProgress", R"({"finished":3,"total":10})");

  boost::system::error_code errorCode;
  const size_t read = asio::read_until(socket, buffer, "\n\n", errorCode);
  ASSERT_FALSE(errorCode) << errorCode.message();

  std::string frame(asio::buffers_begin(buffer.data()), asio::buffers_begin(buffer.data()) + static_cast<long>(read));
  EXPECT_NE(frame.find("event: indexingProgress"), std::string::npos) << frame;
  EXPECT_NE(frame.find(R"(data: {"finished":3,"total":10})"), std::string::npos) << frame;
}

TEST(HttpClient, reportsNulloptWhenNothingIsListening) {
  // Port 1 on loopback: privileged and unbound, so the connect fails rather than hanging.
  http::Client client{"127.0.0.1", 1, Token};
  EXPECT_FALSE(client.request("GET", "/api/v1/stats", {}, 500ms).has_value());
}
