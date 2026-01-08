#include <optional>
#include <string>
#include <vector>

enum class S3AddressingStyle {
    VirtualHosted,
    PathStyle
};

// ListObjects
// https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListObjectsV2.html#API_ListObjectsV2_ResponseSyntax
struct ListObjectsInput {
    std::optional<std::string> ContinuationToken;
    std::optional<std::string> Delimiter;
    std::optional<std::string> EncodingType;
    std::optional<std::string> ExpectedBucketOwner;
    std::optional<bool> FetchOwner;
    std::optional<int> MaxKeys;
    std::optional<std::string> Prefix;
    std::optional<std::string> RequestPayer;
    std::optional<std::string> StartAfter;
};

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
