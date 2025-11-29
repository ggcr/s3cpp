#include <gtest/gtest.h>
#include <s3cpp/auth.h>
#include <s3cpp/httpclient.h>

// TODO(cristian): Skip the functional tests that actually query MinIO if its
// not up

TEST(AUTH, AUTHBasicSign) {
  // create signer
  std::string access_key = "mock_access";
  std::string secret_key = "mock_secret";
  auto signer = AWSSigV4Signer(access_key, secret_key);
  // create httpclient
  HttpClient client{};
  const std::string URL = "https://postman-echo.com/status/200";
  HttpRequest req = client.get(URL);
  signer.sign(req);
  // TODO(cristian)
  EXPECT_TRUE(req.getHeaders().contains("Authorization"));
  // EXPECT_EQ(req.getHeaders().at("Authorization"));
}
