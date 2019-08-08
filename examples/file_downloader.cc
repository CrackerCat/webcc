// Download files.
// This example demonstrates the usage of streamed response.

#include <iostream>

#include "webcc/client_session.h"
#include "webcc/logger.h"

void Help(const char* argv0) {
  std::cout << "Usage: file_downloader <url> <path>" << std::endl;
  std::cout << "E.g.," << std::endl;
  std::cout << "    file_downloader http://httpbin.org/image/jpeg D:/test.jpg"
            << std::endl;
  std::cout << "    file_downloader https://www.google.com/favicon.ico"
            << " D:/test.ico" << std::endl;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    Help(argv[0]);
    return 1;
  }

  std::string url = argv[1];
  std::string path = argv[2];

  WEBCC_LOG_INIT("", webcc::LOG_CONSOLE);

  webcc::ClientSession session;

  try {
    auto r = session.Request(webcc::RequestBuilder{}.Get(url)(),
                             true);  // Stream the response data to file.

    auto file_body = r->file_body();

    file_body->Move(path);

  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
