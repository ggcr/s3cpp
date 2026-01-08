#include <charconv>
#include <expected>
#include <print>
#include <s3cpp/auth.h>
#include <s3cpp/xml.hpp>
#include <stdexcept>
#include <unordered_set>

// ListBucketResult
// https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListObjectsV2.html#API_ListObjectsV2_ResponseSyntax

struct Contents_ {
    std::string ChecksumAlgorithm;
    std::string ChecksumType;
    std::string ETag;
    std::string Key;
    std::string LastModified;
    struct Owner_ {
        std::string DisplayName;
        std::string ID;
    } Owner;
    struct RestoreStatus_ {
        bool IsRestoreInProgress;
        std::string RestoreExpiryDate;
    } RestoreStatus;
    int64_t Size;
    std::string StorageClass;
};

struct CommonPrefix {
    std::string Prefix;
};

struct ListBucketResult {
    bool IsTruncated;
    std::string Marker;
    std::string NextMarker;
    std::vector<Contents_> Contents;
    std::string Name;
    std::string Prefix;
    std::string Delimiter;
    int MaxKeys;
    std::vector<CommonPrefix> CommonPrefixes;
    std::string EncodingType;
    int KeyCount;
    std::string ContinuationToken;
    std::string NextContinuationToken;
    std::string StartAfter;
};

struct Error {
    std::string Code;
    std::string Message;
    std::string BucketName;
    std::string Resource;
    int RequestId;
    std::string HostId;
};

enum class S3AddressingStyle {
    VirtualHosted,
    PathStyle
};

class S3Client {
public:
    // TODO(cristian): We should accept and define the endpoint url here
    S3Client(const std::string& access, const std::string& secret)
        : Client(HttpClient())
        , Signer(AWSSigV4Signer(access, secret))
        , Parser(XMLParser())
        , addressing_style_(S3AddressingStyle::VirtualHosted) {
        // When no endpoint is provided we default to us-east-1 (flashbacks from vietnam)
        endpoint_ = std::format("s3.us-east-1.amazonaws.com");
    }
    S3Client(const std::string& access, const std::string& secret, const std::string& region)
        : Client(HttpClient())
        , Signer(AWSSigV4Signer(access, secret, region))
        , Parser(XMLParser())
        , addressing_style_(S3AddressingStyle::VirtualHosted) {
        // When no endpoint is provided we default to AWS
        endpoint_ = std::format("s3.{}.amazonaws.com", region); // TODO(cristian): Ping to validate region
    }
    S3Client(const std::string& access, const std::string& secret, const std::string& customEndpoint, S3AddressingStyle style)
        : Client(HttpClient())
        , Signer(AWSSigV4Signer(access, secret))
        , Parser(XMLParser())
        , endpoint_(customEndpoint)
        , addressing_style_(style) {
    }

    // TODO(cristian): Implement deserialization
    // TODO(cristian): Wrap onto std::expected
    std::expected<ListBucketResult, Error> GetObject(const std::string& bucket, const std::string& key) {
        std::string url = buildURL(bucket) + std::format("/{}", key);
        HttpRequest req = Client.get(url).header("Host", getHostHeader(bucket));
        Signer.sign(req);
        HttpResponse res = req.execute();
        std::println("{}", res.body());
        std::expected<ListBucketResult, Error> response = deserializeListBucketResult(Parser.parse(res.body()), 1000);
        return response;
    }
    // TODO(cristian): HeadBucket and HeadObject

    std::expected<ListBucketResult, Error> ListObjects(const std::string& bucket) { return ListObjects(bucket, "/", 1000, ""); }
    std::expected<ListBucketResult, Error> ListObjects(const std::string& bucket, const std::string& prefix) { return ListObjects(bucket, prefix, 1000, ""); }
    std::expected<ListBucketResult, Error> ListObjects(const std::string& bucket, const std::string& prefix, int maxKeys) { return ListObjects(bucket, prefix, maxKeys, ""); }
    std::expected<ListBucketResult, Error> ListObjects(const std::string& bucket, const std::string& prefix, int maxKeys, const std::string& continuationToken);

    std::expected<ListBucketResult, Error> deserializeListBucketResult(const std::vector<XMLNode>& nodes, const int maxKeys);
    Error deserializeError(const std::vector<XMLNode>& nodes);

private:
    HttpClient Client;
    AWSSigV4Signer Signer;
    XMLParser Parser;
    std::string endpoint_;
    S3AddressingStyle addressing_style_;

    std::string buildURL(const std::string& bucket) const {
        if (addressing_style_ == S3AddressingStyle::VirtualHosted) {
            // bucket.s3.region.amazonaws.com/key
            return std::format("https://{}.{}", bucket, endpoint_);
        } else {
            // endpoint/bucket/key
            return std::format("http://{}/{}", endpoint_, bucket);
        }
    }

    std::string getHostHeader(const std::string& bucket) const {
        if (addressing_style_ == S3AddressingStyle::VirtualHosted) {
            return std::format("{}.{}", bucket, endpoint_);
        } else {
            return endpoint_;
        }
    }
};

class ListObjectsPaginator {
public:
    ListObjectsPaginator(S3Client& client, const std::string& bucket, const std::string& prefix)
        : client_(client)
        , bucket_(bucket)
        , prefix_(prefix)
        , maxKeys_(1000) { }
    ListObjectsPaginator(S3Client& client, const std::string& bucket, const std::string& prefix, int maxKeys)
        : client_(client)
        , bucket_(bucket)
        , prefix_(prefix)
        , maxKeys_(maxKeys) { }

    bool HasMorePages() const { return hasMorePages_; }

    std::expected<ListBucketResult, Error> NextPage() {
        auto response = client_.ListObjects(bucket_, prefix_, maxKeys_, continuationToken_);
        if (response.has_value()) {
            hasMorePages_ = response.value().IsTruncated;
            continuationToken_ = response.value().NextContinuationToken;
        }
        return response;
    }

private:
    S3Client& client_;
    std::string bucket_;
    std::string prefix_;
    int maxKeys_;
    bool hasMorePages_ = true;
    std::string continuationToken_;
};
