#include "webcc/http_client.h"

#include "boost/algorithm/string.hpp"  // TODO: Remove
#include "boost/date_time/posix_time/posix_time.hpp"

#include "webcc/logger.h"
#include "webcc/utility.h"

using boost::asio::ip::tcp;

namespace webcc {

HttpClient::HttpClient(std::size_t buffer_size, bool ssl_verify)
    : buffer_(buffer_size == 0 ? kBufferSize : buffer_size),
      timer_(io_context_),
      ssl_verify_(ssl_verify),
      timeout_seconds_(kMaxReadSeconds),
      closed_(false),
      timed_out_(false),
      error_(kNoError) {
}

void HttpClient::SetTimeout(int seconds) {
  if (seconds > 0) {
    timeout_seconds_ = seconds;
  }
}

bool HttpClient::Request(const HttpRequest& request,
                         std::size_t buffer_size,
                         bool connect) {
  io_context_.restart();

  response_.reset(new HttpResponse());
  response_parser_.reset(new HttpResponseParser(response_.get()));

  closed_ = false;
  timed_out_ = false;
  error_ = kNoError;

  BufferResizer buffer_resizer(&buffer_, buffer_size);

  if (connect) {
    // No existing socket connection was specified, create a new one.
    if ((error_ = Connect(request)) != kNoError) {
      return false;
    }
  }

  if ((error_ = WriteReqeust(request)) != kNoError) {
    return false;
  }

  if ((error_ = ReadResponse()) != kNoError) {
    return false;
  }

  return true;
}

void HttpClient::Close() {
  if (closed_) {
    return;
  }

  closed_ = true;

  LOG_INFO("Close socket...");

  boost::system::error_code ec;
  socket_->Close(&ec);

  if (ec) {
    LOG_ERRO("Socket close error (%s).", ec.message().c_str());
  }
}

Error HttpClient::Connect(const HttpRequest& request) {
  if (request.url().scheme() == "https") {
    socket_.reset(new HttpSslSocket{ io_context_, ssl_verify_ });
    return DoConnect(request, kPort443);
  } else {
    socket_.reset(new HttpSocket{ io_context_ });
    return DoConnect(request, kPort80);
  }
}

Error HttpClient::DoConnect(const HttpRequest& request,
                            const std::string& default_port) {
  tcp::resolver resolver(io_context_);

  std::string port = request.port(default_port);

  boost::system::error_code ec;
  auto endpoints = resolver.resolve(tcp::v4(), request.host(), port, ec);

  if (ec) {
    LOG_ERRO("Host resolve error (%s): %s, %s.", ec.message().c_str(),
             request.host().c_str(), port.c_str());
    return kHostResolveError;
  }

  LOG_VERB("Connect to server...");

  // Use sync API directly since we don't need timeout control.
  socket_->Connect(request.host(), endpoints, &ec);

  // Determine whether a connection was successfully established.
  if (ec) {
    LOG_ERRO("Socket connect error (%s).", ec.message().c_str());
    Close();
    return kEndpointConnectError;
  }

  LOG_VERB("Socket connected.");

  return kNoError;
}

Error HttpClient::WriteReqeust(const HttpRequest& request) {
  LOG_VERB("HTTP request:\n%s", request.Dump(4, "> ").c_str());

  // NOTE:
  // It doesn't make much sense to set a timeout for socket write.
  // I find that it's almost impossible to simulate a situation in the server
  // side to test this timeout.

  boost::system::error_code ec;

  // Use sync API directly since we don't need timeout control.
  socket_->Write(request, &ec);

  if (ec) {
    LOG_ERRO("Socket write error (%s).", ec.message().c_str());
    Close();
    return kSocketWriteError;
  }

  LOG_INFO("Request sent.");

  return kNoError;
}

Error HttpClient::ReadResponse() {
  LOG_VERB("Read response (timeout: %ds)...", timeout_seconds_);

  timer_.expires_from_now(boost::posix_time::seconds(timeout_seconds_));
  DoWaitTimer();

  Error error = kNoError;
  DoReadResponse(&error);

  if (error == kNoError) {
    LOG_VERB("HTTP response:\n%s", response_->Dump(4, "> ").c_str());
  }

  return error;
}

void HttpClient::DoReadResponse(Error* error) {
  boost::system::error_code ec = boost::asio::error::would_block;

  auto handler = [this, &ec, error](boost::system::error_code inner_ec,
                                    std::size_t length) {
    ec = inner_ec;

    LOG_VERB("Socket async read handler.");

    if (ec || length == 0) {
      StopTimer();
      Close();
      *error = kSocketReadError;
      LOG_ERRO("Socket read error (%s).", ec.message().c_str());
      return;
    }

    LOG_INFO("Read data, length: %u.", length);

    // Parse the response piece just read.
    if (!response_parser_->Parse(buffer_.data(), length)) {
      StopTimer();
      Close();
      *error = kHttpError;
      LOG_ERRO("Failed to parse HTTP response.");
      return;
    }

    if (response_parser_->finished()) {
      // Stop trying to read once all content has been received, because
      // some servers will block extra call to read_some().

      StopTimer();

      if (response_->IsConnectionKeepAlive()) {
        // Close the timer but keep the socket connection.
        LOG_INFO("Keep the socket connection alive.");
      } else {
        Close();
      }

      LOG_INFO("Finished to read and parse HTTP response.");

      // Stop reading.
      return;
    }

    if (!closed_) {
      DoReadResponse(error);
    }
  };

  socket_->AsyncReadSome(std::move(handler), &buffer_);

  // Block until the asynchronous operation has completed.
  do {
    io_context_.run_one();
  } while (ec == boost::asio::error::would_block);
}

void HttpClient::DoWaitTimer() {
  timer_.async_wait(
      std::bind(&HttpClient::OnTimer, this, std::placeholders::_1));
}

void HttpClient::OnTimer(boost::system::error_code ec) {
  if (closed_) {
    return;
  }

  LOG_VERB("On deadline timer.");

  // NOTE: Can't check this:
  //   if (ec == boost::asio::error::operation_aborted) {
  //     LOG_VERB("Deadline timer canceled.");
  //     return;
  //   }

  if (timer_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
    // The deadline has passed.
    // The socket is closed so that any outstanding asynchronous operations
    // are canceled.
    LOG_WARN("HTTP client timed out.");
    timed_out_ = true;
    Close();
    return;
  }

  // Put the actor back to sleep.
  DoWaitTimer();
}

void HttpClient::StopTimer() {
  // Cancel any asynchronous operations that are waiting on the timer.
  LOG_INFO("Cancel deadline timer...");
  timer_.cancel();
}

}  // namespace webcc
