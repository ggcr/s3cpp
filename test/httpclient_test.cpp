#include <curl/curl.h>
#include <curl/easy.h>
#include <exception>
#include <format>
#include <gtest/gtest.h>
#include <s3cpp/httpclient.h>
#include <stdexcept>
#include <string>
#include <type_traits>

TEST(HTTP, AllStatusCodes) {
  HttpClient client{};
  for (auto i : {200, 400, 401, 403, 404, 500, 502}) {
    HttpResponse request =
        client.get(std::format("https://postman-echo.com/status/{}", i));
    EXPECT_EQ(request.status(), i);
    EXPECT_EQ(request.is_ok(), (i >= 200 && i < 300));
    EXPECT_EQ(request.is_client_error(), (i >= 400 && i < 500));
    EXPECT_EQ(request.is_server_error(), (i >= 500 && i < 600));
  }
}

TEST(HTTP, HTTPClientCopy) {
  // duh, the compiler should not generate them
  EXPECT_FALSE(std::is_trivially_copy_constructible<HttpClient>());
  EXPECT_FALSE(std::is_trivially_copy_assignable<HttpClient>());
}

TEST(HTTP, HTTPClientMoveOwnership) {
  EXPECT_TRUE(std::is_move_constructible<HttpClient>());
  EXPECT_TRUE(std::is_move_assignable<HttpClient>());

  HttpClient client1{};
  HttpClient client2(std::move(client1));

  const std::string URL = "https://postman-echo.com/status/200";

  // Invalidated client1 should throw runtime error
  EXPECT_THROW(client1.get(URL), std::runtime_error);
  EXPECT_EQ(client2.get(URL).status(), 200);
}

TEST(HTTP, HTTPBodyNonEmpty) {
  HttpClient client{};
  try {
    HttpResponse request = client.get("https://postman-echo.com/get?foo=bar");
    EXPECT_TRUE(request.is_ok());
    EXPECT_EQ(
        request.body(),
        R"({"args":{"foo":"bar"},"headers":{"host":"postman-echo.com","accept-encoding":"gzip, br","x-forwarded-proto":"https","accept":"*/*"},"url":"https://postman-echo.com/get?foo=bar"})");
  } catch (const std::exception &e) {
    FAIL() << std::format("Request failed with exception: {}", e.what());
  }
}
