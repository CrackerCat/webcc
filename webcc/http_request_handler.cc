#include "webcc/http_request_handler.h"

#include <sstream>

#include "webcc/globals.h"
#include "webcc/http_request.h"
#include "webcc/http_response.h"
#include "webcc/logger.h"

namespace webcc {

void HttpRequestHandler::Enqueue(HttpSessionPtr session) {
  queue_.Push(session);
}

void HttpRequestHandler::Start(std::size_t count) {
  assert(count > 0 && workers_.size() == 0);

  for (std::size_t i = 0; i < count; ++i) {
    workers_.emplace_back(std::bind(&HttpRequestHandler::WorkerRoutine, this));
  }
}

void HttpRequestHandler::Stop() {
  LOG_INFO("Stopping workers...");

  // Close pending sessions.
  for (HttpSessionPtr s = queue_.Pop(); s; s = queue_.Pop()) {
    LOG_INFO("Closing pending session...");
    s->Close();
  }

  // Enqueue a null session to trigger the first worker to stop.
  queue_.Push(HttpSessionPtr());

  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  LOG_INFO("All workers have been stopped.");
}

void HttpRequestHandler::WorkerRoutine() {
  LOG_INFO("Worker is running.");

  for (;;) {
    HttpSessionPtr session = queue_.PopOrWait();

    if (!session) {
      LOG_INFO("Worker is going to stop.");

      // For stopping next worker.
      queue_.Push(HttpSessionPtr());

      // Stop the worker.
      break;
    }

    HandleSession(session);
  }
}

}  // namespace webcc
