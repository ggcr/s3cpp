#include <gtest/gtest.h>
#include <s3cpp/auth.h>
#include <s3cpp/httpclient.h>

// TODO(cristian): Skip the functional tests that actually query MinIO if its
// not up


TEST(AUTH, SHA256HexDigest) {
  auto signer = AWSSigV4Signer("minio_access", "minio_secret");
  EXPECT_EQ(signer.hex(signer.sha256("github.com/ggcr/s3cpp")), "bc088c51b33c2730707dbb528d1d0bfafc59ba56c8c9aa3b8e0dc0c13e3d9b2b");
}

TEST(AUTH, HMACSHA256HexDigest) {
  auto signer = AWSSigV4Signer("minio_access", "minio_secret");
	std::string key = "super-secret-key";
  EXPECT_EQ(signer.hex(signer.HMAC_SHA256(reinterpret_cast<const unsigned char *>(key.c_str()), "github.com/ggcr/s3cpp")), "558084957fb05bb4786ad6791bfbee71e67a11fea964e5dac6bac6b2f749b339");
}

TEST(AUTH, CannonicalGETRequest) {
  // create signer & http client
  auto signer = AWSSigV4Signer("minio_access", "minio_secret");
  HttpClient client{};

  // prepare request
  const std::string host = "s3.amazonaws.com";
  const std::string URI = "/amzn-s3-demo-bucket/myphoto.jpg";
  const std::string URL = std::format("http://{}{}", host, URI);
  const std::string timestamp = signer.getTimestamp();
  const std::string empty_payload_hash =
      "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
  HttpRequest req = client.get(URL)
                        .header("Host", host)
                        .header("X-Amz-Date", timestamp)
                        .header("X-Amz-Content-Sha256", empty_payload_hash);

  const std::string expected_canonical =
      std::format("GET\n"
                  "/amzn-s3-demo-bucket/myphoto.jpg\n"
                  "\n"
                  "host:{}\n"
                  "x-amz-content-sha256:{}\n"
                  "x-amz-date:{}\n"
                  "\n"
                  "host;x-amz-content-sha256;x-amz-date\n"
                  "{}",
                  host, empty_payload_hash, timestamp, empty_payload_hash);

  EXPECT_EQ(signer.createCannonicalRequest(req), expected_canonical);
}
