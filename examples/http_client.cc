#include <iostream>
#include <fstream>

#include "webcc/http_client_session.h"
#include "webcc/logger.h"

#if (defined(WIN32) || defined(_WIN64))
// You need to set environment variable SSL_CERT_FILE properly to enable
// SSL verification.
bool kSslVerify = false;
#else
bool kSslVerify = true;
#endif

void ExampleBasic() {
  webcc::HttpClientSession session;

  auto r = session.Request(webcc::HttpRequestBuilder{}.Get().
                           Url("http://httpbin.org/get").
                           Query("key1", "value1").
                           Query("key2", "value2").
                           Header("Accept", "application/json")
                           ());

  std::cout << r->content() << std::endl;
}

// Use predefined shortcuts.
void ExampleShortcut() {
  webcc::HttpClientSession session;

  auto r = session.Get("http://httpbin.org/get",
                       {"key1", "value1", "key2", "value2"},
                       {"Accept", "application/json"});

  std::cout << r->content() << std::endl;
}

#if WEBCC_ENABLE_SSL

// HTTPS is auto-detected from the URL scheme.
void ExampleHttps() {
  webcc::HttpClientSession session;
  session.set_ssl_verify(kSslVerify);

  auto r = session.Request(webcc::HttpRequestBuilder{}.Get().
                           Url("https://httpbin.org/get").
                           Query("key1", "value1").
                           Header("Accept", "application/json")());

  std::cout << r->content() << std::endl;
}

#endif  // WEBCC_ENABLE_SSL

// Example for testing Keep-Alive connection.
//
// Boost.org doesn't support persistent connection so always includes
// "Connection: Close" header in the response.
// Both Google and GitHub support persistent connection but they don't like
// to include "Connection: Keep-Alive" header in the responses.
//
//   ExampleKeepAlive("http://httpbin.org/get");
//   ExampleKeepAlive("https://www.boost.org/LICENSE_1_0.txt");
//   ExampleKeepAlive("https://www.google.com");
//   ExampleKeepAlive("https://api.github.com/events");
//
void ExampleKeepAlive(const std::string& url) {
  webcc::HttpClientSession session;
  session.set_ssl_verify(kSslVerify);

  // Keep-Alive
  session.Get(url);

  // Close
  session.Get(url, {}, {"Connection", "Close"});

  // Keep-Alive
  session.Get(url);
}

void ExampleCompression() {
  webcc::HttpClientSession session;

  auto r = session.Get("http://httpbin.org/gzip");

  std::cout << r->content() << std::endl;

  r = session.Get("http://httpbin.org/deflate");

  std::cout << r->content() << std::endl;
}

// Get an image from HttpBin.org and save to the given file path.
// E.g., ExampleImage("E:\\example.jpg")
void ExampleImage(const std::string& path) {
  webcc::HttpClientSession session;

  auto r = session.Get("http://httpbin.org/image/jpeg");

  // Or
  //   auto r = session.Get("http://httpbin.org/image", {},
  //                        {"Accept", "image/jpeg"});

  std::ofstream ofs(path, std::ios::binary);
  ofs << r->content();
}

int main() {
  WEBCC_LOG_INIT("", webcc::LOG_CONSOLE);

  webcc::HttpClientSession session;

  try {

    ExampleBasic();

  } catch (const webcc::Exception& e) {
    std::cout << "Exception: " << e.what() << std::endl;
  }

  return 0;
}
