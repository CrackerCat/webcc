#ifndef WEBCC_HTTP_REQUEST_HANDLER_H_
#define WEBCC_HTTP_REQUEST_HANDLER_H_

#include <list>
#include <thread>
#include <vector>

#include "webcc/http_connection.h"
#include "webcc/queue.h"

namespace webcc {

class HttpRequest;
class HttpResponse;

// The common handler for all incoming requests.
class HttpRequestHandler {
public:
  HttpRequestHandler() = default;
  virtual ~HttpRequestHandler() = default;

  HttpRequestHandler(const HttpRequestHandler&) = delete;
  HttpRequestHandler& operator=(const HttpRequestHandler&) = delete;

  // Put the connection into the queue.
  void Enqueue(HttpConnectionPtr connection);

  // Start worker threads.
  void Start(std::size_t count);

  // Clear pending connections from the queue and stop worker threads.
  void Stop();

private:
  void WorkerRoutine();

  // Called by the worker routine.
  virtual void HandleConnection(HttpConnectionPtr connection) = 0;

private:
  Queue<HttpConnectionPtr> queue_;

  std::vector<std::thread> workers_;
};

}  // namespace webcc

#endif  // WEBCC_HTTP_REQUEST_HANDLER_H_
