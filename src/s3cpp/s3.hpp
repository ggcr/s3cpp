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

    void list_objects(const std::string& bucket) {
			return list_objects(bucket, "/");
		}

    void list_objects(const std::string& bucket, const std::string& prefix) {
        // TODO(cristian): Decide what to do with Host, if it will always be the same as the URL,
        // then we can autoamtically create this header on the HttpClient
        // TODO(cristian): This is currently hardcoded to point to MinIO Docker IP-Port...
				
        const std::string empty_payload_hash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
        HttpRequest req = Client.get(std::format("http://127.0.0.1:9000/{}?prefix={}&max-keys=2", bucket, prefix))
                              .header("Host", "127.0.0.1")
                              .header("X-Amz-Content-Sha256", empty_payload_hash);
        Signer.sign(req);

				HttpResponse res = req.execute();

        auto xml_response = Parser.parse(res.body());
				for (const auto& xml_node : xml_response) {
					std::println("{}: {}", xml_node.tag, xml_node.value);
				}

    }

private:
    HttpClient Client;
    AWSSigV4Signer Signer;
    XMLParser Parser;
};
