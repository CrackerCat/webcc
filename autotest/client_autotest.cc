#include <iostream>

#include "gtest/gtest.h"

#include "boost/algorithm/string.hpp"
#include "json/json.h"

#include "webcc/client_session.h"
#include "webcc/logger.h"

// -----------------------------------------------------------------------------

// JSON helper functions (based on jsoncpp).

// Parse a string to JSON object.
static Json::Value StringToJson(const std::string& str) {
  Json::Value json;

  Json::CharReaderBuilder builder;
  std::stringstream stream(str);
  std::string errors;
  if (!Json::parseFromStream(builder, stream, &json, &errors)) {
    std::cerr << errors << std::endl;
  }

  return json;
}

// -----------------------------------------------------------------------------

TEST(ClientTest, Head_RequestFunc) {
  webcc::ClientSession session;

  try {
    auto r = session.Request(webcc::RequestBuilder{}.
                             Head("http://httpbin.org/get")
                             ());

    EXPECT_EQ(webcc::Status::kOK, r->status());
    EXPECT_EQ("OK", r->reason());

    EXPECT_EQ("", r->data());

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
  }
}

TEST(ClientTest, Head_Shortcut) {
  webcc::ClientSession session;

  try {
    auto r = session.Head("http://httpbin.org/get");

    EXPECT_EQ(webcc::Status::kOK, r->status());
    EXPECT_EQ("OK", r->reason());

    EXPECT_EQ("", r->data());

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
  }
}

// Force Accept-Encoding to be "identity" so that HttpBin.org will include
// a Content-Length header in the response.
// This tests that the response with Content-Length while no body could be
// correctly parsed.
TEST(ClientTest, Head_AcceptEncodingIdentity) {
  webcc::ClientSession session;

  try {
    auto r = session.Request(webcc::RequestBuilder{}.
                             Head("http://httpbin.org/get").
                             Header("Accept-Encoding", "identity")
                             ());

    EXPECT_EQ(webcc::Status::kOK, r->status());
    EXPECT_EQ("OK", r->reason());

    EXPECT_TRUE(r->HasHeader(webcc::headers::kContentLength));

    EXPECT_EQ("", r->data());

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
  }
}

// -----------------------------------------------------------------------------

static void AssertGet(webcc::ResponsePtr r) {
  EXPECT_EQ(webcc::Status::kOK, r->status());
  EXPECT_EQ("OK", r->reason());

  Json::Value json = StringToJson(r->data());

  Json::Value args = json["args"];

  EXPECT_EQ(2, args.size());
  EXPECT_EQ("value1", args["key1"].asString());
  EXPECT_EQ("value2", args["key2"].asString());

  Json::Value headers = json["headers"];

  EXPECT_EQ("application/json", headers["Accept"].asString());
  EXPECT_EQ("httpbin.org", headers["Host"].asString());

#if WEBCC_ENABLE_GZIP
  EXPECT_EQ("gzip, deflate", headers["Accept-Encoding"].asString());
#else
  EXPECT_EQ("identity", headers["Accept-Encoding"].asString());
#endif  // WEBCC_ENABLE_GZIP
}

TEST(ClientTest, Get_RequestFunc) {
  webcc::ClientSession session;

  try {
    auto r = session.Request(webcc::RequestBuilder{}.
                             Get("http://httpbin.org/get").
                             Query("key1", "value1").
                             Query("key2", "value2").
                             Header("Accept", "application/json")
                             ());

    AssertGet(r);

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
  }
}

TEST(ClientTest, Get_Shortcut) {
  webcc::ClientSession session;

  try {
    auto r = session.Get("http://httpbin.org/get",
                         { "key1", "value1", "key2", "value2" },
                         { "Accept", "application/json" });

    AssertGet(r);

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
  }
}

#if WEBCC_ENABLE_SSL
TEST(ClientTest, Get_SSL) {
  webcc::ClientSession session;

  try {
    // HTTPS is auto-detected from the URL scheme.
    auto r = session.Request(webcc::RequestBuilder{}.
                             Get("https://httpbin.org/get").
                             Query("key1", "value1").
                             Query("key2", "value2").
                             Header("Accept", "application/json")
                             ());

    AssertGet(r);

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
  }
}
#endif  // WEBCC_ENABLE_SSL

// -----------------------------------------------------------------------------

#if WEBCC_ENABLE_GZIP

// Test Gzip compressed response.
TEST(ClientTest, Compression_Gzip) {
  webcc::ClientSession session;

  try {
    auto r = session.Get("http://httpbin.org/gzip");

    Json::Value json = StringToJson(r->data());

    EXPECT_EQ(true, json["gzipped"].asBool());

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
  }
}

// Test Deflate compressed response.
TEST(ClientTest, Compression_Deflate) {
  webcc::ClientSession session;

  try {
    auto r = session.Get("http://httpbin.org/deflate");

    Json::Value json = StringToJson(r->data());

    EXPECT_EQ(true, json["deflated"].asBool());

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
  }
}

// Test trying to compress the request.
// TODO
TEST(ClientTest, Compression_Request) {
  webcc::ClientSession session;

  try {
    const std::string data = "{'name'='Adam', 'age'=20}";

    // This doesn't really compress the body!
    auto r = session.Request(webcc::RequestBuilder{}.
                             Post("http://httpbin.org/post").
                             Body(data).Json().
                             Gzip()
                             ());

    //Json::Value json = StringToJson(r->data());

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
  }
}
#endif  // WEBCC_ENABLE_GZIP

// -----------------------------------------------------------------------------

// Test persistent (keep-alive) connections.
//
// NOTE:
// Boost.org doesn't support persistent connection and always includes
// "Connection: Close" header in the response.
// Both Google and GitHub support persistent connection but they don't like
// to include "Connection: Keep-Alive" header in the responses.
// URLs:
//   "http://httpbin.org/get";
//   "https://www.boost.org/LICENSE_1_0.txt";
//   "https://www.google.com";
//   "https://api.github.com/events";
//
TEST(ClientTest, KeepAlive) {
  webcc::ClientSession session;

  std::string url = "http://httpbin.org/get";
  try {

    // Keep-Alive by default.
    auto r = session.Get(url);

    using boost::iequals;

    EXPECT_TRUE(iequals(r->GetHeader("Connection"), "Keep-alive"));

    // Close by setting Connection header.
    r = session.Get(url, {}, { "Connection", "Close" });

    EXPECT_TRUE(iequals(r->GetHeader("Connection"), "Close"));

    // Close by using request builder.
    r = session.Request(webcc::RequestBuilder{}.
                        Get(url).KeepAlive(false)
                        ());

    EXPECT_TRUE(iequals(r->GetHeader("Connection"), "Close"));

    // Keep-Alive explicitly by using request builder.
    r = session.Request(webcc::RequestBuilder{}.
                        Get(url).KeepAlive(true)
                        ());

    EXPECT_TRUE(iequals(r->GetHeader("Connection"), "Keep-alive"));

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
  }
}

// -----------------------------------------------------------------------------

// Get a JPEG image.
TEST(ClientTest, GetImageJpeg) {
  webcc::ClientSession session;

  try {

    auto r = session.Get("http://httpbin.org/image/jpeg");

    // Or
    //   auto r = session.Get("http://httpbin.org/image", {},
    //                        {"Accept", "image/jpeg"});

    //std::ofstream ofs(path, std::ios::binary);
    //ofs << r->data();

    // TODO: Verify the response is a valid JPEG image.

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
  }
}

// -----------------------------------------------------------------------------

TEST(ClientTest, Post_RequestFunc) {
  webcc::ClientSession session;

  try {
    const std::string data = "{'name'='Adam', 'age'=20}";

    auto r = session.Request(webcc::RequestBuilder{}.
                             Post("http://httpbin.org/post").
                             Body(data).Json()
                             ());

    EXPECT_EQ(webcc::Status::kOK, r->status());
    EXPECT_EQ("OK", r->reason());

    Json::Value json = StringToJson(r->data());

    EXPECT_EQ(data, json["data"].asString());

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
  }
}

TEST(ClientTest, Post_Shortcut) {
  webcc::ClientSession session;

  try {
    const std::string data = "{'name'='Adam', 'age'=20}";

    auto r = session.Post("http://httpbin.org/post", std::string(data), true);

    EXPECT_EQ(webcc::Status::kOK, r->status());
    EXPECT_EQ("OK", r->reason());

    Json::Value json = StringToJson(r->data());

    EXPECT_EQ(data, json["data"].asString());

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
  }
}

#if (WEBCC_ENABLE_GZIP && WEBCC_ENABLE_SSL)
// NOTE: Most servers don't support compressed requests!
TEST(ClientTest, Post_Gzip) {
  webcc::ClientSession session;

  try {
    // Use Boost.org home page as the POST data.
    auto r1 = session.Get("https://www.boost.org/");
    const std::string& data = r1->data();

    auto r2 = session.Request(webcc::RequestBuilder{}.
                              Post("http://httpbin.org/post").
                              Body(data).Gzip()
                              ());

    EXPECT_EQ(webcc::Status::kOK, r2->status());
    EXPECT_EQ("OK", r2->reason());

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
  }
}
#endif  // (WEBCC_ENABLE_GZIP && WEBCC_ENABLE_SSL)

// -----------------------------------------------------------------------------

int main(int argc, char* argv[]) {
  // Set webcc::LOG_CONSOLE to enable logging.
  WEBCC_LOG_INIT("", 0);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
