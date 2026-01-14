#include "s3cpp/httpclient.h"
#include <expected>
#include <s3cpp/s3.h>

std::expected<ListObjectsResult, Error> S3Client::ListObjects(const std::string& bucket, const ListObjectsInput& options) {
    // Silent-ly accept maxKeys > 1000, even though we will return 1K at most
    // Pagination is opt-in as in the Go SDK, the user must be aware of this

    // Build URL with query parameters
    std::string url = std::format("{}?list-type=2", buildURL(bucket));

    if (options.Prefix.has_value())
        url += std::format("&prefix={}", options.Prefix.value());

    int maxKeys = options.MaxKeys.value_or(1000);
    url += std::format("&max-keys={}", maxKeys);

    if (options.ContinuationToken.has_value())
        url += std::format("&continuation-token={}", options.ContinuationToken.value());
    if (options.Delimiter.has_value())
        url += std::format("&delimiter={}", options.Delimiter.value());
    if (options.EncodingType.has_value())
        url += std::format("&encoding-type={}", options.EncodingType.value());
    if (options.StartAfter.has_value())
        url += std::format("&start-after={}", options.StartAfter.value());
    if (options.FetchOwner.has_value() && options.FetchOwner.value())
        url += "&fetch-owner=true";

    HttpRequest req = Client.get(url).header("Host", getHostHeader(bucket));

    // opt headers
    if (options.ExpectedBucketOwner.has_value())
        req.header("x-amz-expected-bucket-owner", options.ExpectedBucketOwner.value());
    if (options.RequestPayer.has_value())
        req.header("x-amz-request-payer", options.RequestPayer.value());

    Signer.sign(req);
    HttpResponse res = req.execute();

    const std::vector<XMLNode>& XMLBody = Parser.parse(res.body());

    if (res.is_ok()) {
        return deserializeListBucketResult(XMLBody, maxKeys);
    }
    return std::unexpected<Error>(deserializeError(XMLBody));
}

std::expected<ListObjectsResult, Error> S3Client::deserializeListBucketResult(const std::vector<XMLNode>& nodes, const int maxKeys) {
    ListObjectsResult result;
    result.Contents.reserve(maxKeys);
    result.CommonPrefixes.reserve(maxKeys);

    result.Contents.push_back(Contents_ {});
    result.CommonPrefixes.push_back(CommonPrefix {});

    int contentsIdx = 0;
    int commonPrefixesIdx = 0;

    // To keep track when we need to append an element
    std::vector<std::string_view> seenContents;
    std::vector<std::string_view> seenCommonPrefix;

    for (const auto& node : nodes) {
        /* Sigh... no reflection */

        // Check if we've seen this tag before in the current object
        if (node.tag.contains("ListBucketResult.Contents")) {
            if (std::find(seenContents.begin(), seenContents.end(), node.tag) != seenContents.end()) {
                result.Contents.push_back(Contents_ {});
                seenContents.clear();
                contentsIdx++;
            }
        } else if (node.tag.contains("ListBucketResult.CommonPrefix")) {
            if (std::find(seenCommonPrefix.begin(), seenCommonPrefix.end(), node.tag) != seenCommonPrefix.end()) {
                result.CommonPrefixes.push_back(CommonPrefix {});
                seenCommonPrefix.clear();
                commonPrefixesIdx++;
            }
        }

        if (node.tag == "ListBucketResult.IsTruncated") {
            result.IsTruncated = Parser.parseBool(std::move(node.value));
        } else if (node.tag == "ListBucketResult.Marker") {
            result.Marker = std::move(node.value);
        } else if (node.tag == "ListBucketResult.NextMarker") {
            result.NextMarker = std::move(node.value);
        } else if (node.tag == "ListBucketResult.Name") {
            result.Name = std::move(node.value);
        } else if (node.tag == "ListBucketResult.Prefix") {
            result.Prefix = std::move(node.value);
        } else if (node.tag == "ListBucketResult.Delimiter") {
            result.Delimiter = std::move(node.value);
        } else if (node.tag == "ListBucketResult.MaxKeys") {
            result.MaxKeys = Parser.parseNumber<int>(std::move(node.value));
        } else if (node.tag == "ListBucketResult.EncodingType") {
            result.EncodingType = std::move(node.value);
        } else if (node.tag == "ListBucketResult.KeyCount") {
            result.KeyCount = Parser.parseNumber<int>(std::move(node.value));
        } else if (node.tag == "ListBucketResult.ContinuationToken") {
            result.ContinuationToken = std::move(node.value);
        } else if (node.tag == "ListBucketResult.NextContinuationToken") {
            result.NextContinuationToken = std::move(node.value);
        } else if (node.tag == "ListBucketResult.StartAfter") {
            result.StartAfter = std::move(node.value);
        } else if (node.tag == "ListBucketResult.Contents.ChecksumAlgorithm") {
            result.Contents[contentsIdx].ChecksumAlgorithm = std::move(node.value);
        } else if (node.tag == "ListBucketResult.Contents.ChecksumType") {
            result.Contents[contentsIdx].ChecksumType = std::move(node.value);
        } else if (node.tag == "ListBucketResult.Contents.ETag") {
            result.Contents[contentsIdx].ETag = std::move(node.value);
        } else if (node.tag == "ListBucketResult.Contents.Key") {
            result.Contents[contentsIdx].Key = std::move(node.value);
        } else if (node.tag == "ListBucketResult.Contents.LastModified") {
            result.Contents[contentsIdx].LastModified = std::move(node.value);
        } else if (node.tag == "ListBucketResult.Contents.Owner.DisplayName") {
            result.Contents[contentsIdx].Owner.DisplayName = std::move(node.value);
        } else if (node.tag == "ListBucketResult.Contents.Owner.ID") {
            result.Contents[contentsIdx].Owner.ID = std::move(node.value);
        } else if (node.tag == "ListBucketResult.Contents.RestoreStatus.IsRestoreInProgress") {
            result.Contents[contentsIdx].RestoreStatus.IsRestoreInProgress = Parser.parseBool(node.value);
        } else if (node.tag == "ListBucketResult.Contents.RestoreStatus.RestoreExpiryDate") {
            result.Contents[contentsIdx].RestoreStatus.RestoreExpiryDate = std::move(node.value);
        } else if (node.tag == "ListBucketResult.Contents.Size") {
            result.Contents[contentsIdx].Size = Parser.parseNumber<long>(node.value);
        } else if (node.tag == "ListBucketResult.Contents.StorageClass") {
            result.Contents[contentsIdx].StorageClass = std::move(node.value);
        } else if (node.tag == "ListBucketResult.CommonPrefixes.Prefix") {
            result.CommonPrefixes[commonPrefixesIdx].Prefix = std::move(node.value);
        } else {
            // Detect and parse error
            // Note(cristian): This fallback should not be needed as we have
            // the HTTP status codes for this, however, I like it
            if (node.tag.substr(0, 6) == "Error.") {
                return std::unexpected<Error>(deserializeError(nodes));
            }
            throw std::runtime_error(std::format("No case for ListBucketResult response found for: {}", node.tag));
        }

        // Add already seen fields
        if (node.tag.contains("ListBucketResult.Contents")) {
            seenContents.push_back(node.tag);
        } else if (node.tag.contains("ListBucketResult.CommonPrefix")) {
            seenCommonPrefix.push_back(node.tag);
        }
    }

    // Remove the initial empty object if it was never populated
    if (!result.Contents.empty() && result.Contents[0].Key.empty()) {
        result.Contents.erase(result.Contents.begin());
    }
    if (!result.CommonPrefixes.empty() && result.CommonPrefixes[0].Prefix.empty()) {
        result.CommonPrefixes.erase(result.CommonPrefixes.begin());
    }

    return result;
}

std::expected<std::string, Error> S3Client::GetObject(const std::string& bucket, const std::string& key, const GetObjectInput& options) {
    std::string url = buildURL(bucket) + std::format("/{}", key);

    HttpRequest req = Client.get(url).header("Host", getHostHeader(bucket));

    // opt headers
    if (options.Range.has_value())
        req.header("Range", options.Range.value());
    if (options.If_Match.has_value())
        req.header("If-Match", options.If_Match.value());
    if (options.If_None_Match.has_value())
        req.header("If-None-Match", options.If_None_Match.value());
    if (options.If_Modified_Since.has_value())
        req.header("If-Modified-Since", options.If_Modified_Since.value());
    if (options.If_Unmodified_Since.has_value())
        req.header("If-Unmodified-Since", options.If_Unmodified_Since.value());

    Signer.sign(req);
    HttpResponse res = req.execute();

    if (res.is_ok()) {
        return res.body();
    }
    return std::unexpected<Error>(deserializeError(Parser.parse(res.body())));
}

std::expected<PutObjectResult, Error> S3Client::PutObject(const std::string& bucket, const std::string& key, const std::string& body, const PutObjectInput& options) {
    // TODO(cristian): For now let's support only string body

    std::string url = buildURL(bucket) + std::format("/{}", key);

    HttpBodyRequest req = Client.put(url)
                              .header("Host", getHostHeader(bucket))
                              .body(body);

    // opt headers
    // ...

    Signer.sign(req);
    HttpResponse res = req.execute();

    if (res.is_ok()) {
        return deserializePutObjectResult(res.headers());
    }
    const std::vector<XMLNode>& XMLBody = Parser.parse(res.body());
    return std::unexpected<Error>(deserializeError(XMLBody));
}

std::expected<DeleteObjectResult, Error> S3Client::DeleteObject(const std::string& bucket, const std::string& key, const DeleteObjectInput& options) {
    std::string url = buildURL(bucket) + std::format("/{}", key);
    if (options.versionId.has_value())
        url += std::format("?versionId={}", options.versionId.value());

    HttpRequest req = Client.get(url).header("Host", getHostHeader(bucket));

    // opt headers
    if (options.MFA.has_value())
        req.header("x-amz-mfa", options.MFA.value());
    if (options.RequestPayer.has_value())
        req.header("x-amz-request-payer", options.RequestPayer.value());
    if (options.ByPassGovernanceRetention.has_value())
        req.header("x-amz-bypass-governance-retention", options.ByPassGovernanceRetention.value());
    if (options.ExpectedBucketOwner.has_value())
        req.header("x-amz-expected-bucket-owner", options.ExpectedBucketOwner.value());
    if (options.If_Match.has_value())
        req.header("If-Match", options.If_Match.value());
    if (options.If_MatchLastModifiedTime.has_value())
        req.header("x-amz-if-match-last-modified-time", options.If_MatchLastModifiedTime.value());
    if (options.If_MatchSize.has_value())
        req.header("x-amz-if-match-size", options.If_MatchSize.value());

    Signer.sign(req);
    HttpResponse res = req.execute();

    if (res.is_ok()) {
        return deserializeDeleteObjectResult(res.headers());
    }
    return std::unexpected<Error>(deserializeError(Parser.parse(res.body())));
}

std::expected<CreateBucketResult, Error> S3Client::CreateBucket(
    const std::string& bucket,
    const CreateBucketConfiguration& configuration,
    const CreateBucketInput& options) {

    std::string url = buildURL(bucket);

    HttpBodyRequest req = Client.put(url).header("Host", getHostHeader(bucket));

    // opt headers
    if (options.ACL.has_value())
        req.header("x-amz-acl", std::move(options.ACL.value()));
    if (options.GrantFullControl.has_value())
        req.header("x-amz-grant-full-control", std::move(options.GrantFullControl.value()));
    if (options.GrantRead.has_value())
        req.header("x-amz-grant-read", std::move(options.GrantRead.value()));
    if (options.GrantReadACP.has_value())
        req.header("x-amz-grant-read-acp", std::move(options.GrantReadACP.value()));
    if (options.GrantWrite.has_value())
        req.header("x-amz-grant-write", std::move(options.GrantWrite.value()));
    if (options.GrantWriteACP.has_value())
        req.header("x-amz-grant-write-acp", std::move(options.GrantWriteACP.value()));
    if (options.ObjectLockEnabledForBucket.has_value()) {
        auto booleanValueStr = (options.ObjectLockEnabledForBucket.value() == true) ? "true" : "false";
        req.header("x-amz-bucket-object-lock-enabled", std::move(booleanValueStr));
    }
    if (options.ObjectOwnership.has_value())
        req.header("x-amz-object-ownership", std::move(options.ObjectOwnership.value()));

    // XML request body
    // https://docs.aws.amazon.com/AmazonS3/latest/API/API_CreateBucket.html#API_CreateBucket_RequestSyntax
    std::string createBucketReqBodyXML = R"(<CreateBucketConfiguration xmlns="http://s3.amazonaws.com/doc/2006-03-01/">)";
    // TODO(cristian): is this if statement needed?
    // That is, can we do <LocationConstraint></LocationConstraint> if it's not provided?
    if (!configuration.LocationConstraint.empty())
        createBucketReqBodyXML += std::format("<LocationConstraint>{}</LocationConstraint>", configuration.LocationConstraint);
    if (!configuration.Location.Name.empty() || !configuration.Location.Type.empty()) {
        createBucketReqBodyXML += "<Location>";
        if (!configuration.Location.Name.empty())
            createBucketReqBodyXML += std::format("<Name>{}</Name>", configuration.Location.Name);
        if (!configuration.Location.Type.empty())
            createBucketReqBodyXML += std::format("<Type>{}</Type>", configuration.Location.Type);
        createBucketReqBodyXML += "</Location>";
    }
    if (!configuration.Bucket.DataRedundancy.empty() || !configuration.Bucket.Type.empty()) {
        createBucketReqBodyXML += "<Bucket>";
        if (!configuration.Bucket.DataRedundancy.empty())
            createBucketReqBodyXML += std::format("<DataRedundancy>{}</DataRedundancy>", configuration.Bucket.DataRedundancy);
        if (!configuration.Bucket.Type.empty())
            createBucketReqBodyXML += std::format("<Type>{}</Type>", configuration.Bucket.Type);
        createBucketReqBodyXML += "</Bucket>";
    }
    if (!configuration.Tags.empty()) {
        createBucketReqBodyXML += "<Tags>";
        for (const auto& tag : configuration.Tags) {
            createBucketReqBodyXML += "<Tag>";
            createBucketReqBodyXML += std::format("<Key>{}</Key>", tag.Key);
            createBucketReqBodyXML += std::format("<Value>{}</Value>", tag.Value);
            createBucketReqBodyXML += "</Tag>";
        }
        createBucketReqBodyXML += "</Tags>";
    }
    createBucketReqBodyXML += "</CreateBucketConfiguration>";
    req.body(std::move(createBucketReqBodyXML));

    Signer.sign(req);
    HttpResponse res = req.execute();

    if (res.is_ok()) {
        return deserializeCreateBucketResult(res.headers());
    }
    const std::vector<XMLNode>& XMLBody = Parser.parse(res.body());
    return std::unexpected<Error>(deserializeError(XMLBody));
}

Error S3Client::deserializeError(const std::vector<XMLNode>& nodes) {
    Error error;

    for (const auto& node : nodes) {
        /* Sigh... no reflection */

        if (node.tag == "Error.Code") {
            error.Code = std::move(node.value);
        } else if (node.tag == "Error.Message") {
            error.Message = std::move(node.value);
        } else if (node.tag == "Error.Resource") {
            error.Resource = std::move(node.value);
        } else if (node.tag == "Error.RequestId") {
            error.RequestId = Parser.parseNumber<int>(std::move(node.value));
        } else {
            continue;
        }
    }

    return error;
}

std::expected<PutObjectResult, Error> S3Client::deserializePutObjectResult(const std::map<std::string, std::string, LowerCaseCompare>& headers) {
    PutObjectResult result;

    for (const auto& [header, value] : headers) {
        /* Sigh... no reflection */
        if (header == "ETag")
            result.ETag = std::move(value);
        else if (header == "Expiration")
            result.Expiration = std::move(value);
        else if (header == "ChecksumCRC32")
            result.ChecksumCRC32 = std::move(value);
        else if (header == "ChecksumCRC32C")
            result.ChecksumCRC32C = std::move(value);
        else if (header == "ChecksumCRC64NVME")
            result.ChecksumCRC64NVME = std::move(value);
        else if (header == "ChecksumSHA1")
            result.ChecksumSHA1 = std::move(value);
        else if (header == "ChecksumSHA256")
            result.ChecksumSHA256 = std::move(value);
        else if (header == "ChecksumType")
            result.ChecksumType = std::move(value);
        else if (header == "ServerSideEncryption")
            result.ServerSideEncryption = std::move(value);
        else if (header == "VersionId")
            result.VersionId = std::move(value);
        else if (header == "SSECustomerAlgorithm")
            result.SSECustomerAlgorithm = std::move(value);
        else if (header == "SSECustomerKeyMD5")
            result.SSECustomerKeyMD5 = std::move(value);
        else if (header == "SSEKMSKeyId")
            result.SSEKMSKeyId = std::move(value);
        else if (header == "SSEKMSEncryptionContext")
            result.SSEKMSEncryptionContext = std::move(value);
        else if (header == "BucketKeyEnabled")
            result.BucketKeyEnabled = Parser.parseBool(value);
        else if (header == "Size")
            result.Size = Parser.parseNumber<int64_t>(value);
        else if (header == "RequestCharged")
            result.RequestCharged = std::move(value);
        else {
            continue;
            // To debug:
            // throw std::runtime_error(std::format("No case for PutObjectResult response found for: {}", header));
        }
    }

    return result;
}

std::expected<DeleteObjectResult, Error> S3Client::deserializeDeleteObjectResult(const std::map<std::string, std::string, LowerCaseCompare>& headers) {
    DeleteObjectResult result;
    for (const auto& [header, value] : headers) {
        if (header == "x-amz-version-id")
            result.versionId = std::move(value);
        else if (header == "x-amz-delete-marker")
            result.DeleteMarker = std::move(value);
        else if (header == "x-amz-request-charged")
            result.RequestCharged = std::move(value);
        else {
            continue;
        }
    }
    return result;
}

std::expected<CreateBucketResult, Error> S3Client::deserializeCreateBucketResult(const std::map<std::string, std::string, LowerCaseCompare>& headers) {
    CreateBucketResult result;
    for (const auto& [header, value] : headers) {
        if (header == "Location")
            result.Location = std::move(value);
        else if (header == "x-amz-bucket-arn")
            result.BucketARN = std::move(value);
        else {
            continue;
        }
    }
    return result;
}
