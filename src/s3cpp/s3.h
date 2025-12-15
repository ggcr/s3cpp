#include <s3cpp/auth.h>
#include <s3cpp/xml.h>

class S3Client {
public:
    S3Client(const std::string& access, const std::string& secret)
        : Client(HttpClient())
        , Signer(AWSSigV4Signer(access, secret))
        , Parser(XMLParser()) { }
    S3Client(const std::string& access, const std::string& secret, const std::string& region)
        : Client(HttpClient())
        , Signer(AWSSigV4Signer(access, secret, region))
        , Parser(XMLParser()) { }

private:
    HttpClient Client;
    AWSSigV4Signer Signer;
    XMLParser Parser;
};
