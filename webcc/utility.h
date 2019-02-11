#ifndef WEBCC_UTILITY_H_
#define WEBCC_UTILITY_H_

#include <iosfwd>
#include <string>
#include <vector>

#include "boost/asio/ip/tcp.hpp"

namespace webcc {

// If |port| is empty, try to extract it from |host| (separated by ':').
void AdjustHostPort(std::string& host, std::string& port);

void PrintEndpoint(std::ostream& ostream,
                   const boost::asio::ip::tcp::endpoint& endpoint);

void PrintEndpoints(
    std::ostream& ostream,
    const boost::asio::ip::tcp::resolver::results_type& endpoints);

std::string EndpointToString(const boost::asio::ip::tcp::endpoint& endpoint);

// Get the timestamp for HTTP Date header field.
// E.g., Wed, 21 Oct 2015 07:28:00 GMT
// See: https://tools.ietf.org/html/rfc7231#section-7.1.1.2
std::string GetHttpDateTimestamp();

// Resize a buffer in ctor and restore its original size in dtor.
struct BufferResizer {
  BufferResizer(std::vector<char>* buffer, std::size_t new_size)
    : buffer_(buffer), old_size_(buffer->size()) {
    if (new_size != 0 && new_size != old_size_) {
      buffer_->resize(new_size);
    }
  }

  ~BufferResizer() {
    if (buffer_->size() != old_size_) {
      buffer_->resize(old_size_);
    }
  }

  std::vector<char>* buffer_;
  std::size_t old_size_;
};

}  // namespace webcc

#endif  // WEBCC_UTILITY_H_
