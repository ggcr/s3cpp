#include <expected>
#include <print>
#include <s3cpp/auth.h>
#include <s3cpp/xml.hpp>

// ListObjects
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

struct GetObjectInput {
	std::optional<std::string> If_Match;
	std::optional<std::string> If_Modified_Since;
	std::optional<std::string> If_None_Match;
	std::optional<std::string> If_Unmodified_Since;
	std::optional<int> partNumber;
	std::optional<std::string> Range; // e.g. bytes=0-9
	std::optional<std::string> response_cache_control;
	std::optional<std::string> response_content_disposition;
	std::optional<std::string> response_content_encoding;
	std::optional<std::string> response_content_language;
	std::optional<std::string> response_content_type;
	std::optional<std::string> response_expires;
	std::optional<std::string> versionId;
};

struct ListObjectsResult {
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

// REST generic error
// https://docs.aws.amazon.com/AmazonS3/latest/API/ErrorResponses.html#RESTErrorResponses
struct Error {
    std::string Code;
    std::string Message;
    std::string Resource;
    int RequestId;
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

	 // S3 operations
	 // TODO(cristian): ListObjectsV2 missing URI params:
	 // - Bucket 
	 // - Continuation token
	 // - Delimiter
	 // - EncodingType
	 // - ExpectedBucketOwner
	 // - FetchOwner
	 // - MaxKeys
	 // - OptionalObjectAttributes
	 // - Prefix
	 // - RequestPayer
	 // - StartAfter
    std::expected<ListObjectsResult, Error> ListObjects(const std::string& bucket) { return ListObjects(bucket, "/", 1000, ""); }
    std::expected<ListObjectsResult, Error> ListObjects(const std::string& bucket, const std::string& prefix) { return ListObjects(bucket, prefix, 1000, ""); }
    std::expected<ListObjectsResult, Error> ListObjects(const std::string& bucket, const std::string& prefix, int maxKeys) { return ListObjects(bucket, prefix, maxKeys, ""); }
    std::expected<ListObjectsResult, Error> ListObjects(const std::string& bucket, const std::string& prefix, int maxKeys, const std::string& continuationToken);

    std::expected<std::string, Error> GetObject(const std::string& bucket, const std::string& key);
    std::expected<std::string, Error> GetObject(const std::string& bucket, const std::string& key, const GetObjectInput& opt);
	 // TODO(cristian): Add all overloading needed for different params
    // TODO(cristian): HeadBucket and HeadObject, PutObject, CreateBucket

	 // S3 responses
    std::expected<ListObjectsResult, Error> deserializeListBucketResult(const std::vector<XMLNode>& nodes, const int maxKeys);
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

    std::expected<ListObjectsResult, Error> NextPage() {
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
