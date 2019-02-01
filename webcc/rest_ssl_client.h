#ifndef WEBCC_REST_SSL_CLIENT_H_
#define WEBCC_REST_SSL_CLIENT_H_

#include <cassert>
#include <string>
#include <utility>  // for move()

#include "webcc/globals.h"
#include "webcc/http_request.h"
#include "webcc/http_response.h"
#include "webcc/http_ssl_client.h"

namespace webcc {

class RestSslClient {
 public:
  // If |port| is empty, |host| will be checked to see if it contains port or
  // not (separated by ':').
  explicit RestSslClient(const std::string& host,
                         const std::string& port = "",
                         bool ssl_verify = true);

  ~RestSslClient() = default;

  WEBCC_DELETE_COPY_ASSIGN(RestSslClient);

  void SetTimeout(int seconds) {
    http_client_.SetTimeout(seconds);
  }

  // NOTE:
  // The return value of the following methods (Get, Post, etc.) only indicates
  // if the socket communication is successful or not. Check error() and
  // timed_out() for more information if it's failed. Check response_status()
  // instead for the HTTP status code.

  inline bool Get(const std::string& url) {
    return Request(kHttpGet, url, "");
  }

  inline bool Post(const std::string& url, std::string&& content) {
    return Request(kHttpPost, url, std::move(content));
  }

  inline bool Put(const std::string& url, std::string&& content) {
    return Request(kHttpPut, url, std::move(content));
  }

  inline bool Patch(const std::string& url, std::string&& content) {
    return Request(kHttpPatch, url, std::move(content));
  }

  inline bool Delete(const std::string& url) {
    return Request(kHttpDelete, url, "");
  }

  HttpResponsePtr response() const {
    return http_client_.response();
  }

  int response_status() const {
    assert(response());
    return response()->status();
  }

  const std::string& response_content() const {
    assert(response());
    return response()->content();
  }

  bool timed_out() const {
    return http_client_.timed_out();
  }

  Error error() const {
    return http_client_.error();
  }

private:
  bool Request(const std::string& method,
               const std::string& url,
               std::string&& content);

  std::string host_;
  std::string port_;

  HttpSslClient http_client_;
};

}  // namespace webcc

#endif  // WEBCC_REST_SSL_CLIENT_H_
