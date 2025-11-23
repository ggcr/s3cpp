#include <gtest/gtest.h>
#include <s3cpp/auth.h>

TEST(AUTH, DefaultRegion) {
  std::string access_key = "minio_access";
  std::string secret_key = "minio_secret";
  auto auth = AWSSigV4Signer(access_key, secret_key);
  EXPECT_EQ(auth.getRegion(), "us-east-1");
}

TEST(AUTH, CustomRegion) {
  std::string access_key = "minio_access";
  std::string secret_key = "minio_secret";
  std::string region = "eu-west-2";
  auto auth = AWSSigV4Signer(access_key, secret_key, region);
  EXPECT_EQ(auth.getRegion(), region);
}
