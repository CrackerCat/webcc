#ifndef WEBCC_GLOBALS_H_
#define WEBCC_GLOBALS_H_

#include <cassert>
#include <exception>
#include <iosfwd>
#include <string>

#include "webcc/config.h"

// -----------------------------------------------------------------------------
// Macros

#ifdef _MSC_VER

#if _MSC_VER <= 1800  // VS 2013

// Does the compiler support "= default" for move copy constructor and
// move assignment operator?
#define WEBCC_DEFAULT_MOVE_COPY_ASSIGN 0

#define WEBCC_NOEXCEPT

#else

#define WEBCC_DEFAULT_MOVE_COPY_ASSIGN 1

#define WEBCC_NOEXCEPT noexcept

#endif  // _MSC_VER <= 1800

#else

// GCC, Clang, etc.

#define WEBCC_DEFAULT_MOVE_COPY_ASSIGN 1

#define WEBCC_NOEXCEPT noexcept

#endif  // _MSC_VER

namespace webcc {

// -----------------------------------------------------------------------------

const char* const kCRLF = "\r\n";

const std::size_t kInvalidLength = std::string::npos;

// Default timeout for reading response.
const int kMaxReadSeconds = 30;

// Max size of the HTTP body to dump/log.
// If the HTTP, e.g., response, has a very large content, it will be truncated
// when dumped/logged.
const std::size_t kMaxDumpSize = 2048;

// Default buffer size for socket reading.
const std::size_t kBufferSize = 1024;

// Why 1400? See the following page:
// https://www.itworld.com/article/2693941/why-it-doesn-t-make-sense-to-
// gzip-all-content-from-your-web-server.html
const std::size_t kGzipThreshold = 1400;

// -----------------------------------------------------------------------------

namespace methods {

// HTTP methods (verbs) in string.
// Don't use enum to avoid converting back and forth.

const char* const kGet = "GET";
const char* const kHead = "HEAD";
const char* const kPost = "POST";
const char* const kPut = "PUT";
const char* const kDelete = "DELETE";
const char* const kConnect = "CONNECT";
const char* const kOptions = "OPTIONS";
const char* const kTrace = "TRACE";
const char* const kPatch = "PATCH";

}  // namespace methods

// HTTP status codes.
// Don't use "enum class" for converting to/from int easily.
// The full list is available here:
//   https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
enum Status {
  kOK = 200,
  kCreated = 201,
  kAccepted = 202,
  kNoContent = 204,
  kNotModified = 304,
  kBadRequest = 400,
  kNotFound = 404,
  kInternalServerError = 500,
  kNotImplemented = 501,
  kServiceUnavailable = 503,
};

namespace headers {

// NOTE: Field names are case-insensitive.
//   See https://stackoverflow.com/a/5259004 for more details.

const char* const kHost = "Host";
const char* const kDate = "Date";
const char* const kAuthorization = "Authorization";
const char* const kContentType = "Content-Type";
const char* const kContentLength = "Content-Length";
const char* const kContentEncoding = "Content-Encoding";
const char* const kContentDisposition = "Content-Disposition";
const char* const kConnection = "Connection";
const char* const kTransferEncoding = "Transfer-Encoding";
const char* const kAccept = "Accept";
const char* const kAcceptEncoding = "Accept-Encoding";
const char* const kUserAgent = "User-Agent";
const char* const kServer = "Server";

}  // namespace headers

namespace media_types {

// See the following link for the full list of media types:
//   https://www.iana.org/assignments/media-types/media-types.xhtml

const char* const kApplicationJson = "application/json";
const char* const kApplicationSoapXml = "application/soap+xml";
const char* const kTextPlain = "text/plain";
const char* const kTextXml = "text/xml";

// Get media type from file extension.
std::string FromExtension(const std::string& ext);

}  // namespace media_types

namespace charsets {

const char* const kUtf8 = "utf-8";

}  // namespace charsets

enum class ContentEncoding {
  kUnknown,
  kGzip,
  kDeflate,
};

// -----------------------------------------------------------------------------

// Error (or exception) for the client.
class Error {
public:
  enum Code {
    kOK = 0,
    kSyntaxError,
    kResolveError,
    kConnectError,
    kHandshakeError,
    kSocketReadError,
    kSocketWriteError,
    kParseError,
    kFileError,
  };

public:
  Error(Code code = kOK, const std::string& message = "")
      : code_(code), message_(message), timeout_(false) {
  }

  Code code() const { return code_; }

  const std::string& message() const { return message_; }

  void Set(Code code, const std::string& message) {
    code_ = code;
    message_ = message;
  }

  bool timeout() const { return timeout_; }

  void set_timeout(bool timeout) { timeout_ = timeout; }

  operator bool() const { return code_ != kOK; }

private:
  Code code_;
  std::string message_;
  bool timeout_;
};

std::ostream& operator<<(std::ostream& os, const Error& error);

}  // namespace webcc

#endif  // WEBCC_GLOBALS_H_
