#ifndef S3CPP_AUTH
#define S3CPP_AUTH

#include "s3cpp/httpclient.h"

class AWSSigV4Signer {
public:
  AWSSigV4Signer(const std::string &access, const std::string &secret)
      : httpclient(HttpClient()), access_key(std::move(access)),
        secret_key(std::move(secret)), aws_region("us-east-1") {
    // sign-in
  }
  AWSSigV4Signer(const std::string &access, const std::string &secret,
                 const std::string &region)
      : httpclient(HttpClient()), access_key(std::move(access)),
        aws_region(std::move(region)), secret_key(std::move(secret)) {
    // sign-in
  }

  const std::string &getRegion() const { return aws_region; }

private:
  HttpClient httpclient;
  std::string access_key;
  std::string secret_key;
  std::string aws_region;
};

#endif
