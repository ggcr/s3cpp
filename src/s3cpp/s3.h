#include <charconv>
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

    ListBucketResult GetObject(const std::string& bucket, const std::string& key) {
			return ListBucketResult{};
		}

    ListBucketResult ListObjects(const std::string& bucket) { return ListObjects(bucket, "/", 1000, ""); }
    ListBucketResult ListObjects(const std::string& bucket, const std::string& prefix) { return ListObjects(bucket, prefix, 1000, ""); }
    ListBucketResult ListObjects(const std::string& bucket, const std::string& prefix, int maxKeys) { return ListObjects(bucket, prefix, maxKeys, ""); }
    ListBucketResult ListObjects(const std::string& bucket, const std::string& prefix, int maxKeys, const std::string& continuationToken);

    ListBucketResult deserializeListBucketResult(const std::vector<XMLNode>& nodes, const int maxKeys);

private:
    HttpClient Client;
    AWSSigV4Signer Signer;
    XMLParser Parser;
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

    ListBucketResult NextPage() {
        ListBucketResult response = client_.ListObjects(bucket_, prefix_, maxKeys_, continuationToken_);
        hasMorePages_ = response.IsTruncated;
        continuationToken_ = response.NextContinuationToken;
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
