#include <chrono>
#include <curl/curl.h>
#include <curl/easy.h>
#include <exception>
#include <format>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <s3cpp/httpclient.h>
#include <stdexcept>
#include <string>
#include <type_traits>

TEST(HTTP, AllStatusCodes) {
  HttpClient client{};
  for (auto i : {200, 400, 401, 403, 404, 500, 502}) {
    HttpResponse request =
        client.get(std::format("https://postman-echo.com/status/{}", i))
            .execute();
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
  EXPECT_THROW(client1.get(URL).execute(), std::runtime_error);
  EXPECT_EQ(client2.get(URL).execute().status(), 200);
}

TEST(HTTP, HTTPBodyNonEmpty) {
  HttpClient client{};
  try {
    HttpResponse request =
        client.get("https://postman-echo.com/get?foo=bar").execute();
    EXPECT_TRUE(request.is_ok());
    EXPECT_THAT(request.body(), testing::HasSubstr("\"foo\":\"bar\""));

  } catch (const std::exception &e) {
    FAIL() << std::format("Request failed with exception: {}", e.what());
  }
}

TEST(HTTP, HTTPHandleTimeout) {
  HttpClient client{};
  try {
    auto response =
        client.get("https://postman-echo.com/delay/10").timeout(1).execute();
    FAIL() << "Client not handling 1s timeout on a 10s delayed request";
  } catch (const std::exception &e) {
    SUCCEED(); // libcurl error for request: Timeout was reached
  }
}

TEST(HTTP, HTTPRequestInDifferentPhases) {
  HttpClient client{};
  // TODO(cristian): Allow for floating points on timeout?
  HttpRequest req = client.get("https://postman-echo.com/get").timeout(1);
  auto responses = std::vector<HttpResponse>{};
  try {
    HttpResponse resp1 = req.execute();
    responses.push_back(resp1);
  } catch (const std::exception &e) {
    SUCCEED(); // libcurl error for request: Timeout was reached
  }
  try {
    HttpResponse resp2 = req.execute();
    responses.push_back(resp2);
  } catch (const std::exception &e) {
    SUCCEED(); // libcurl error for request: Timeout was reached
  }
  ASSERT_EQ(responses.size(), 2);
  ASSERT_EQ(responses[0], responses[1]);
}

TEST(HTTP, HTTPRequestCastTimeout) {
  HttpClient client{};

  // int (broadcasted to long)
  HttpRequest request = client.get("foo").timeout(1).timeout(2);
  EXPECT_EQ(request.getTimeout(), 2);

  // std::chrono
  request.timeout(std::chrono::hours(1));
  EXPECT_EQ(request.getTimeout(), 3600);
}

// TODO(cristian): Headers extensive tests

TEST(HTTP, HTTPClientDefaultHeadersOverwrittenByRequestHeaders) {
  // HttpClient allows setting headers once through a initializer list
  // this headers are then merged (and overriden) with the HttpRequest headers
  HttpClient client({
      {"Authentication", "mytoken"},
      {"User-Agent", "foo"},
  });

  // Send request with custom user-agent header, should override client header
  for (const auto &val : {"foo", "bar"}) {

    // Change the user-agent header from the request
    HttpRequest request = client.get("https://postman-echo.com/get")
                              .timeout(10)
                              .header("User-Agent", val);

    try {
      auto resp = request.execute();

      // request should work + having the new user-agent header each time
      EXPECT_TRUE(resp.is_ok());
      EXPECT_THAT(resp.body(), testing::HasSubstr(
                                   std::format("\"user-agent\":\"{}\"", val)));

    } catch (const std::exception &e) {
      FAIL() << std::format("Request failed: {}", e.what());
    }
  }
}

TEST(HTTP, HTTPRemoveHeader) {
  HttpClient client(std::unordered_map<std::string, std::string>({
      {"User-Agent", "client"},
  }));

  // On libcurl passing an empty header removes it
  //     curl_slist_append(list, "Header:");
  // in which case, when merging request and client headers, if we remove a
  // request one, the client one should also be removed!

  try {
    // Removes the User-Agent header
    auto resp = client.get("https://postman-echo.com/get")
                    .timeout(10)
                    .header("User-Agent", "")
                    .execute();
    EXPECT_THAT(resp.body(), testing::Not(testing::HasSubstr("\"user-agent\":\"client\"")));
  } catch (const std::exception &e) {
    FAIL() << std::format("Request failed: {}", e.what());
  }
}

TEST(HTTP, HTTPHead) {
  HttpClient client{};
  HttpResponse resp = client.head("https://postman-echo.com/get?foo0=bar1&foo2=bar2").execute();
	EXPECT_TRUE(resp.body().empty());
}

