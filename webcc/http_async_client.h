#ifndef WEBCC_HTTP_ASYNC_CLIENT_H_
#define WEBCC_HTTP_ASYNC_CLIENT_H_

#include <functional>
#include <memory>
#include <vector>

#include "boost/asio/deadline_timer.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/tcp.hpp"

#include "webcc/globals.h"
#include "webcc/http_request.h"
#include "webcc/http_response.h"
#include "webcc/http_response_parser.h"

namespace webcc {

// Response callback.
typedef std::function<void(HttpResponsePtr, Error, bool)> HttpResponseCallback;

// HTTP client session in asynchronous mode.
// A request will return without waiting for the response, the callback handler
// will be invoked when the response is received or timeout occurs.
// Don't use the same HttpAsyncClient object in multiple threads.
class HttpAsyncClient : public std::enable_shared_from_this<HttpAsyncClient> {
 public:
  // The |buffer_size| is the bytes of the buffer for reading response.
  // 0 means default value (e.g., 1024) will be used.
  explicit HttpAsyncClient(boost::asio::io_context& io_context,
                           std::size_t buffer_size = 0);

  WEBCC_DELETE_COPY_ASSIGN(HttpAsyncClient);

  // Set the timeout seconds for reading response.
  // The |seconds| is only effective when greater than 0.
  void SetTimeout(int seconds);

  // Asynchronously connect to the server, send the request, read the response,
  // and call the |response_callback| when all these finish.
  void Request(HttpRequestPtr request, HttpResponseCallback response_callback);

  // Called by the user to cancel the request.
  void Stop();

 private:
  using tcp = boost::asio::ip::tcp;

  void OnResolve(boost::system::error_code ec,
                 tcp::resolver::results_type results);

  void OnConnect(boost::system::error_code ec, tcp::endpoint endpoint);

  void DoWrite();
  void OnWrite(boost::system::error_code ec);

  void DoRead();
  void OnRead(boost::system::error_code ec, std::size_t length);

  void DoWaitDeadline();
  void OnDeadline(boost::system::error_code ec);

  // Terminate all the actors to shut down the connection. 
  void DoStop();

  tcp::resolver resolver_;
  tcp::socket socket_;

  std::shared_ptr<HttpRequest> request_;
  std::vector<char> buffer_;

  HttpResponsePtr response_;
  std::unique_ptr<HttpResponseParser> response_parser_;
  HttpResponseCallback response_callback_;

  // Timer for the timeout control.
  boost::asio::deadline_timer deadline_;

  // Maximum seconds to wait before the client cancels the operation.
  // Only for receiving response from server.
  int timeout_seconds_;

  // Request stopped due to timeout or socket error.
  bool stopped_;

  // If the error was caused by timeout or not.
  // Will be passed to the response handler/callback.
  bool timed_out_;
};

typedef std::shared_ptr<HttpAsyncClient> HttpAsyncClientPtr;

}  // namespace webcc

#endif  // WEBCC_HTTP_ASYNC_CLIENT_H_
