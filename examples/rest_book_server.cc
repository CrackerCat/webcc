#include <iostream>
#include <list>
#include <string>
#include <thread>
#include <vector>

#include "json/json.h"

#include "webcc/logger.h"
#include "webcc/response_builder.h"
#include "webcc/server.h"

#include "examples/common/book.h"
#include "examples/common/book_json.h"

#if (defined(_WIN32) || defined(_WIN64))
#if defined(_DEBUG) && defined(WEBCC_ENABLE_VLD)
#pragma message ("< include vld.h >")
#include "vld/vld.h"
#pragma comment(lib, "vld")
#endif
#endif

// -----------------------------------------------------------------------------

static BookStore g_book_store;

static void Sleep(int seconds) {
  if (seconds > 0) {
    LOG_INFO("Sleep %d seconds...", seconds);
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
  }
}

// -----------------------------------------------------------------------------

class BookListView : public webcc::View {
public:
  explicit BookListView(int sleep_seconds) : sleep_seconds_(sleep_seconds) {
  }

  webcc::ResponsePtr Handle(webcc::RequestPtr request) override {
    if (request->method() == "GET") {
      return Get(request);
    }

    if (request->method() == "POST") {
      return Post(request);
    }

    return {};
  }

private:
  // Get a list of books based on query parameters.
  webcc::ResponsePtr Get(webcc::RequestPtr request);

  // Create a new book.
  webcc::ResponsePtr Post(webcc::RequestPtr request);

private:
  // Sleep some seconds before send back the response.
  // For testing timeout control in client side.
  int sleep_seconds_;
};

// -----------------------------------------------------------------------------

// The URL is like '/books/{BookID}', and the 'args' parameter
// contains the matched book ID.
class BookDetailView : public webcc::View {
public:
  explicit BookDetailView(int sleep_seconds) : sleep_seconds_(sleep_seconds) {
  }

  webcc::ResponsePtr Handle(webcc::RequestPtr request) override {
    if (request->method() == "GET") {
      return Get(request);
    }

    if (request->method() == "PUT") {
      return Put(request);
    }

    if (request->method() == "DELETE") {
      return Delete(request);
    }

    return {};
  }

private:
  // Get the detailed information of a book.
  webcc::ResponsePtr Get(webcc::RequestPtr request);

  // Update a book.
  webcc::ResponsePtr Put(webcc::RequestPtr request);

  // Delete a book.
  webcc::ResponsePtr Delete(webcc::RequestPtr request);

private:
  // Sleep some seconds before send back the response.
  // For testing timeout control in client side.
  int sleep_seconds_;
};

// -----------------------------------------------------------------------------

// Return all books as a JSON array.
webcc::ResponsePtr BookListView::Get(webcc::RequestPtr request) {
  Sleep(sleep_seconds_);

  Json::Value json(Json::arrayValue);

  for (const Book& book : g_book_store.books()) {
    json.append(BookToJson(book));
  }

  return webcc::ResponseBuilder{}.OK().Body(JsonToString(json)).Json().
      Utf8()();
}

webcc::ResponsePtr BookListView::Post(webcc::RequestPtr request) {
  Sleep(sleep_seconds_);

  Book book;
  if (JsonStringToBook(request->data(), &book)) {
    std::string id = g_book_store.AddBook(book);

    Json::Value json;
    json["id"] = id;

    return webcc::ResponseBuilder{}.Created().Body(JsonToString(json)).
        Json().Utf8()();
  } else {
    // Invalid JSON
    return webcc::ResponseBuilder{}.BadRequest()();
  }
}

// -----------------------------------------------------------------------------

webcc::ResponsePtr BookDetailView::Get(webcc::RequestPtr request) {
  Sleep(sleep_seconds_);

  if (request->args().size() != 1) {
    // Using kNotFound means the resource specified by the URL cannot be found.
    // kBadRequest could be another choice.
    return webcc::ResponseBuilder{}.NotFound()();
  }

  const std::string& book_id = request->args()[0];

  const Book& book = g_book_store.GetBook(book_id);
  if (book.IsNull()) {
    return webcc::ResponseBuilder{}.NotFound()();
  }

  return webcc::ResponseBuilder{}.OK().Body(BookToJsonString(book)).Json().
      Utf8()();
}

webcc::ResponsePtr BookDetailView::Put(webcc::RequestPtr request) {
  Sleep(sleep_seconds_);

  if (request->args().size() != 1) {
    return webcc::ResponseBuilder{}.NotFound()();
  }

  const std::string& book_id = request->args()[0];

  Book book;
  if (!JsonStringToBook(request->data(), &book)) {
    return webcc::ResponseBuilder{}.BadRequest()();
  }

  book.id = book_id;
  g_book_store.UpdateBook(book);

  return webcc::ResponseBuilder{}.OK()();
}

webcc::ResponsePtr BookDetailView::Delete(webcc::RequestPtr request) {
  Sleep(sleep_seconds_);

  if (request->args().size() != 1) {
    return webcc::ResponseBuilder{}.NotFound()();
  }

  const std::string& book_id = request->args()[0];

  if (!g_book_store.DeleteBook(book_id)) {
    return webcc::ResponseBuilder{}.NotFound()();
  }

  return webcc::ResponseBuilder{}.OK()();
}

// -----------------------------------------------------------------------------

void Help(const char* argv0) {
  std::cout << "Usage: " << argv0 << " <port> [seconds]" << std::endl;
  std::cout << "If |seconds| is provided, the server will sleep these seconds "
               "before sending back each response."
            << std::endl;
  std::cout << "  E.g.," << std::endl;
  std::cout << "    " << argv0 << " 8080" << std::endl;
  std::cout << "    " << argv0 << " 8080 3" << std::endl;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    Help(argv[0]);
    return 1;
  }

  WEBCC_LOG_INIT("", webcc::LOG_CONSOLE);

  std::uint16_t port = static_cast<std::uint16_t>(std::atoi(argv[1]));

  int sleep_seconds = 0;
  if (argc >= 3) {
    sleep_seconds = std::atoi(argv[2]);
  }

  std::size_t workers = 2;

  try {
    webcc::Server server(port, workers);

    server.Route("/books",
                 std::make_shared<BookListView>(sleep_seconds),
                 { "GET", "POST" });

    server.Route(webcc::R("/books/(\\d+)"),
                 std::make_shared<BookDetailView>(sleep_seconds),
                 { "GET", "PUT", "DELETE" });

    server.Run();

  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
