#ifndef WEBCC_HTTP_REQUEST_H_
#define WEBCC_HTTP_REQUEST_H_

#include <memory>
#include <string>
#include <vector>

#include "webcc/http_message.h"
#include "webcc/url.h"

namespace webcc {

class HttpRequest;
typedef std::shared_ptr<HttpRequest> HttpRequestPtr;

class HttpRequest : public HttpMessage {
public:
  HttpRequest() = default;

  HttpRequest(const std::string& method, const std::string& url)
      : method_(method), url_(url) {
  }

  ~HttpRequest() override = default;

  void set_method(const std::string& method) {
    method_ = method;
  }

  void set_url(const std::string& url) {
    url_.Init(url);
  }

  // Add URL query parameter.
  void AddParameter(const std::string& key, const std::string& value) {
    url_.AddParameter(key, value);
  }

  void set_buffer_size(std::size_t buffer_size) {
    buffer_size_ = buffer_size;
  }

  void set_ssl_verify(bool ssl_verify) {
    ssl_verify_ = ssl_verify;
  }

  const std::string& method() const {
    return method_;
  }

  const Url& url() const {
    return url_;
  }

  const std::string& host() const {
    return url_.host();
  }

  const std::string& port() const {
    return url_.port();
  }

  std::string port(const std::string& default_port) const {
    return port().empty() ? default_port : port();
  }

  // TODO: Remove
  std::size_t buffer_size() const {
    return buffer_size_;
  }

  // TODO: Remove
  bool ssl_verify() const {
    return ssl_verify_;
  }

  // Prepare payload.
  // Compose start line, set Host header, etc.
  bool Prepare() final;

private:
  std::string method_;
  Url url_;

  // Verify the certificate of the peer or not (for HTTPS).
  bool ssl_verify_ = true;

  // The size of the buffer for reading response.
  // 0 means default value will be used.
  std::size_t buffer_size_ = 0;
};

}  // namespace webcc

#endif  // WEBCC_HTTP_REQUEST_H_
