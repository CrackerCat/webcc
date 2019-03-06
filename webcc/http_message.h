#ifndef WEBCC_HTTP_MESSAGE_H_
#define WEBCC_HTTP_MESSAGE_H_

#include <cassert>
#include <string>
#include <utility>  // for move()
#include <vector>

#include "boost/asio/buffer.hpp"  // for const_buffer

#include "webcc/globals.h"

namespace webcc {

// -----------------------------------------------------------------------------

typedef std::pair<std::string, std::string> HttpHeader;

class HttpMessage;

std::ostream& operator<<(std::ostream& os, const HttpMessage& message);

// -----------------------------------------------------------------------------

class HttpHeaderDict {
public:
  std::size_t size() const {
    return headers_.size();
  }

  void Add(const std::string& key, const std::string& value);

  void Add(std::string&& key, std::string&& value);

  bool Has(const std::string& key) const;

  const HttpHeader& Get(std::size_t i) const {
    assert(i < size());
    return headers_[i];
  }

  const std::vector<HttpHeader>& data() const {
    return headers_;
  }

private:
  std::vector<HttpHeader> headers_;
};

// -----------------------------------------------------------------------------

// Base class for HTTP request and response messages.
class HttpMessage {
public:
  HttpMessage() : content_length_(kInvalidLength) {
  }

  virtual ~HttpMessage() = default;

  const std::string& start_line() const {
    return start_line_;
  }

  void set_start_line(const std::string& start_line) {
    start_line_ = start_line;
  }

  std::size_t content_length() const {
    return content_length_;
  }

  const std::string& content() const {
    return content_;
  }

  void SetHeader(const std::string& key, const std::string& value) {
    headers_.Add(key, value);
  }

  void SetHeader(std::string&& key, std::string&& value) {
    headers_.Add(std::move(key), std::move(value));
  }

  // E.g., "text/html", "application/json; charset=utf-8", etc.
  void SetContentType(const std::string& media_type,
                      const std::string& charset);

  // TODO: Remove parameter |set_length|.
  void SetContent(std::string&& content, bool set_length);

  void SetContentInAppJsonUtf8(std::string&& content, bool set_length);

  // Make the message (e.g., update start line).
  // Must be called before ToBuffers()!
  virtual bool Prepare() = 0;

  // Convert the message into a vector of buffers. The buffers do not own the
  // underlying memory blocks, therefore the message object must remain valid
  // and not be changed until the write operation has completed.
  std::vector<boost::asio::const_buffer> ToBuffers() const;

  // Dump to output stream.
  void Dump(std::ostream& os, std::size_t indent = 0,
            const std::string& prefix = "") const;

  // Dump to string, only used by logger.
  std::string Dump(std::size_t indent = 0,
                   const std::string& prefix = "") const;

protected:
  void SetContentLength(std::size_t content_length) {
    content_length_ = content_length;
    SetHeader(http::headers::kContentLength, std::to_string(content_length));
  }

  // Start line with trailing CRLF.
  std::string start_line_;

  std::size_t content_length_;

  HttpHeaderDict headers_;

  std::string content_;
};

}  // namespace webcc

#endif  // WEBCC_HTTP_MESSAGE_H_
