#include <s3cpp/auth.h>
#include <s3cpp/xml.hpp>

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

    void list_objects(const std::string& bucket, const std::string& path) {
        const std::string host = "127.0.0.1:9000";
        const std::string URI = "/";
        const std::string URL = std::format("http://{}{}", host, URI);
        const std::string empty_payload_hash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
				// Should the signer be allowed to add headers?
				// If so, can it add the timestamp?
        HttpRequest req = Client.get(URL)
                              .header("Host", host)
                              .header("X-Amz-Date", Signer.getTimestamp())
                              .header("X-Amz-Content-Sha256", empty_payload_hash);
        Signer.sign(req);
    }

private:
    HttpClient Client;
    AWSSigV4Signer Signer;
    XMLParser Parser;
};
