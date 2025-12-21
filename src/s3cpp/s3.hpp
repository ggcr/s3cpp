#include <s3cpp/auth.h>
#include <s3cpp/xml.hpp>

class S3Client {
public:
	// TODO(cristian): We should accept and define the endpoint url here
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
        // TODO(cristian): Decide what to do with the Host header
				
        HttpRequest req = Client.get(std::format("http://127.0.0.1:9000/{}?prefix={}", bucket, prefix))
                              .header("Host", "127.0.0.1");
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
