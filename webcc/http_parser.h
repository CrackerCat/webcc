#ifndef WEBCC_HTTP_PARSER_H_
#define WEBCC_HTTP_PARSER_H_

#include <string>

#include "webcc/globals.h"

namespace webcc {

class HttpMessage;

// HttpParser parses HTTP request and response.
class HttpParser {
public:
  explicit HttpParser(HttpMessage* message);

  virtual ~HttpParser() = default;

  HttpParser(const HttpParser&) = delete;
  HttpParser& operator=(const HttpParser&) = delete;

  bool finished() const { return finished_; }

  std::size_t content_length() const { return content_length_; }

  bool Parse(const char* data, std::size_t length);

public:
  // Parse headers from pending data.
  // Return false only on syntax errors.
  bool ParseHeaders();

  // Get next line (using delimiter CRLF) from the pending data.
  // The line will not contain a trailing CRLF.
  // If |remove| is true, the line, as well as the trailing CRLF, will be erased
  // from the pending data.
  bool NextPendingLine(std::size_t off, std::string* line, bool remove);

  virtual bool ParseStartLine(const std::string& line) = 0;

  bool ParseHeaderLine(const std::string& line);

  bool ParseFixedContent();

  bool ParseChunkedContent();
  bool ParseChunkSize();

  void Finish();

  void AppendContent(const char* data, std::size_t count);
  void AppendContent(const std::string& data);

  bool IsContentFull() const;

  // The result HTTP message.
  HttpMessage* message_;

  // Data waiting to be parsed.
  std::string pending_data_;

  // Temporary data and helper flags for parsing.
  std::size_t content_length_;
  std::string content_;
  bool start_line_parsed_;
  bool content_length_parsed_;
  bool header_ended_;
  bool chunked_;
  std::size_t chunk_size_;
  bool finished_;
};

}  // namespace webcc

#endif  // WEBCC_HTTP_PARSER_H_
