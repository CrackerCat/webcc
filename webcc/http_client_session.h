#ifndef WEBCC_HTTP_CLIENT_SESSION_H_
#define WEBCC_HTTP_CLIENT_SESSION_H_

#include <string>
#include <vector>

#include "webcc/http_request_args.h"
#include "webcc/http_response.h"

namespace webcc {

class HttpClientSession {
public:
  HttpClientSession();

  ~HttpClientSession() = default;

  void set_content_type(const std::string& content_type) {
    content_type_ = content_type;
  }

  void set_charset(const std::string& charset) {
    charset_ = charset;
  }

  void AddHeader(const std::string& key, const std::string& value) {
    headers_.Add(key, value);
  }

  HttpResponsePtr Request(HttpRequestArgs&& args);

  HttpResponsePtr Get(const std::string& url,
                      std::vector<std::string>&& parameters = {},
                      std::vector<std::string>&& headers = {},
                      HttpRequestArgs&& args = HttpRequestArgs());

  HttpResponsePtr Post(const std::string& url, std::string&& data, bool json,
                       std::vector<std::string>&& headers = {},
                       HttpRequestArgs&& args = HttpRequestArgs());

private:
  void InitHeaders();

  // E.g., "application/json".
  std::string content_type_;

  // E.g., "utf-8".
  std::string charset_;

  // Headers for each request sent from this session.
  HttpHeaderDict headers_;
};

}  // namespace webcc

#endif  // WEBCC_HTTP_CLIENT_SESSION_H_
