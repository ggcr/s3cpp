#ifndef S3CPP_AUTH
#define S3CPP_AUTH

#include "s3cpp/httpclient.h"
#include <string>

class AWSSigV4Signer {
public:
  AWSSigV4Signer(const std::string &access, const std::string &secret)
      : access_key(std::move(access)), secret_key(std::move(secret)),
        aws_region("us-east-1") {}
  AWSSigV4Signer(const std::string &access, const std::string &secret,
                 const std::string &region)
      : access_key(std::move(access)), aws_region(std::move(region)),
        secret_key(std::move(secret)) {}

  void sign(HttpRequest &request) {
    request.header("Authorization", "AWS4-HMAC-SHA256");
    // Compute date in YYYYMMDD format
    const std::chrono::time_point now{std::chrono::system_clock::now()};
    const std::chrono::year_month_day ymd{
        std::chrono::floor<std::chrono::days>(now)};
    const std::string request_date = std::format(
        "{:04d}{:02d}{:02d}", static_cast<int>(ymd.year()),
        static_cast<unsigned>(ymd.month()), static_cast<unsigned>(ymd.day()));
    const std::string credential =
        "Credential: " +
        std::format("{}/{}/{}/s3/aws4_request", access_key, request_date,
                    aws_region) +
        ",";
  }

private:
  std::string access_key;
  std::string secret_key;
  std::string aws_region;
};

#endif
