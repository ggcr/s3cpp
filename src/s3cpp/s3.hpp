#include <charconv>
#include <print>
#include <s3cpp/auth.h>
#include <s3cpp/xml.hpp>
#include <stdexcept>
#include <unordered_set>

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

    ListBucketResult list_objects(const std::string& bucket) { return list_objects(bucket, "/", 1000); }
    ListBucketResult list_objects(const std::string& bucket, const std::string& prefix) { return list_objects(bucket, prefix, 1000); }
    ListBucketResult list_objects(const std::string& bucket, const std::string& prefix, int maxKeys) {
        HttpRequest req = Client.get(std::format("http://127.0.0.1:9000/{}?prefix={}&max-keys={}", bucket, prefix, maxKeys)).header("Host", "127.0.0.1");
        Signer.sign(req);
        HttpResponse res = req.execute();
        return deserializeListBucketResult(Parser.parse(res.body()));
    }

    ListBucketResult deserializeListBucketResult(const std::vector<XMLNode>& nodes) {
        // TODO(cristian): Detect and parse errors
        ListBucketResult response;
				response.Contents.push_back(Contents{});
				response.CommonPrefixes.push_back(CommonPrefix{});
				int contentsIdx = 0; 
				int commonPrefixesIdx = 0; 

				// To keep track when we need to append an element
				std::unordered_set<std::string> contentsKeySet; 
				std::unordered_set<std::string> commonPrefixKeySet; 

        for (const auto& node : nodes) {
            /* Sigh... no reflection */
						
						// Check if we've seen this tag before in the current object
            if (contentsKeySet.contains(node.tag)) {
							response.Contents.push_back(Contents{});
							contentsKeySet.clear();
							contentsIdx++;
						} else if (commonPrefixKeySet.contains(node.tag)) {
							response.CommonPrefixes.push_back(CommonPrefix{});
							commonPrefixKeySet.clear();
							commonPrefixesIdx++;
						}

            if (node.tag == "ListBucketResult.IsTruncated") {
                response.IsTruncated = Parser.parseBool(std::move(node.value));
            } else if (node.tag == "ListBucketResult.Marker") {
                response.Marker = std::move(node.value);
            } else if (node.tag == "ListBucketResult.NextMarker") {
                response.NextMarker = std::move(node.value);
            } else if (node.tag == "ListBucketResult.Name") {
                response.Name = std::move(node.value);
            } else if (node.tag == "ListBucketResult.Prefix") {
                response.Prefix = std::move(node.value);
            } else if (node.tag == "ListBucketResult.Delimiter") {
                response.Delimiter = std::move(node.value);
            } else if (node.tag == "ListBucketResult.MaxKeys") {
                response.MaxKeys = Parser.parseNumber<int>(std::move(node.value));
            } else if (node.tag == "ListBucketResult.EncodingType") {
                response.EncodingType = std::move(node.value);
            } else if (node.tag == "ListBucketResult.Contents.ChecksumAlgorithm") {
                response.Contents[contentsIdx].ChecksumAlgorithm = std::move(node.value);
            } else if (node.tag == "ListBucketResult.Contents.ChecksumType") {
                response.Contents[contentsIdx].ChecksumType = std::move(node.value);
            } else if (node.tag == "ListBucketResult.Contents.ETag") {
                response.Contents[contentsIdx].ETag = std::move(node.value);
            } else if (node.tag == "ListBucketResult.Contents.Key") {
                response.Contents[contentsIdx].Key = std::move(node.value);
            } else if (node.tag == "ListBucketResult.Contents.LastModified") {
                response.Contents[contentsIdx].LastModified = std::move(node.value);
            } else if (node.tag == "ListBucketResult.Contents.Owner.DisplayName") {
                response.Contents[contentsIdx].Owner.DisplayName = std::move(node.value);
            } else if (node.tag == "ListBucketResult.Contents.Owner.ID") {
                response.Contents[contentsIdx].Owner.ID = std::move(node.value);
            } else if (node.tag == "ListBucketResult.Contents.RestoreStatus.IsRestoreInProgress") {
                response.Contents[contentsIdx].RestoreStatus.IsRestoreInProgress = Parser.parseBool(node.value);
            } else if (node.tag == "ListBucketResult.Contents.RestoreStatus.RestoreExpiryDate") {
                response.Contents[contentsIdx].RestoreStatus.RestoreExpiryDate = std::move(node.value);
            } else if (node.tag == "ListBucketResult.Contents.Size") {
                response.Contents[contentsIdx].Size = Parser.parseNumber<long>(node.value);
            } else if (node.tag == "ListBucketResult.Contents.StorageClass") {
                response.Contents[contentsIdx].StorageClass = std::move(node.value);
            } else {
                throw std::runtime_error(std::format("No case for ListBucketResult response found for: {}", node.tag));
            }

						// Add already seen fields
            if (node.tag.contains("ListBucketResult.Contents")) {
							contentsKeySet.insert(node.tag);
						} else if (node.tag.contains("ListBucketResult.CommonPrefix")) {
							commonPrefixKeySet.insert(node.tag);
						}
        }
        return response;
    }

private:
    HttpClient Client;
    AWSSigV4Signer Signer;
    XMLParser Parser;
};
