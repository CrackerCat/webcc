#ifndef WEBCC_HTTP_CLIENT_BASE_H_
#define WEBCC_HTTP_CLIENT_BASE_H_

#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "boost/asio/deadline_timer.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/tcp.hpp"

#include "webcc/globals.h"
#include "webcc/http_request.h"
#include "webcc/http_response.h"
#include "webcc/http_response_parser.h"

namespace webcc {

// The base class of synchronous HTTP clients.
// In synchronous mode, a request won't return until the response is received
// or timeout occurs.
// Please don't use the same client object in multiple threads.
class HttpClientBase {
public:
  // The |buffer_size| is the bytes of the buffer for reading response.
  // 0 means default value (e.g., 1024) will be used.
  explicit HttpClientBase(std::size_t buffer_size = 0);

  virtual ~HttpClientBase() = default;

  HttpClientBase(const HttpClientBase&) = delete;
  HttpClientBase& operator=(const HttpClientBase&) = delete;

  // Set the timeout seconds for reading response.
  // The |seconds| is only effective when greater than 0.
  void SetTimeout(int seconds);

  // Connect to server, send request, wait until response is received.
  // Set |buffer_size| to non-zero to use a different buffer size for this
  // specific request.
  bool Request(const HttpRequest& request, std::size_t buffer_size = 0);

  HttpResponsePtr response() const { return response_; }

  int response_status() const {
    assert(response_);
    return response_->status();
  }

  const std::string& response_content() const {
    assert(response_);
    return response_->content();
  }

  bool timed_out() const { return timed_out_; }

  Error error() const { return error_; }

public:
  typedef boost::asio::ip::tcp::resolver::results_type Endpoints;

  typedef std::function<void(boost::system::error_code, std::size_t)>
      ReadHandler;

  virtual Error Connect(const HttpRequest& request) = 0;

  Error DoConnect(const HttpRequest& request, const std::string& default_port);

  Error WriteReqeust(const HttpRequest& request);

  Error ReadResponse();

  void DoReadResponse(Error* error);

  void DoWaitDeadline();
  void OnDeadline(boost::system::error_code ec);

  void Stop();

  virtual void SocketConnect(const Endpoints& endpoints,
                             boost::system::error_code* ec) = 0;

  virtual void SocketWrite(const HttpRequest& request,
                           boost::system::error_code* ec) = 0;

  virtual void SocketAsyncReadSome(ReadHandler&& handler) = 0;

  virtual void SocketClose(boost::system::error_code* ec) = 0;

  boost::asio::io_context io_context_;

  std::vector<char> buffer_;

  HttpResponsePtr response_;
  std::unique_ptr<HttpResponseParser> response_parser_;

  // Timer for the timeout control.
  boost::asio::deadline_timer deadline_;

  // Maximum seconds to wait before the client cancels the operation.
  // Only for reading response from server.
  int timeout_seconds_;

  // Request stopped due to timeout or socket error.
  bool stopped_;

  // If the error was caused by timeout or not.
  bool timed_out_;

  Error error_;
};

}  // namespace webcc

#endif  // WEBCC_HTTP_CLIENT_BASE_H_
