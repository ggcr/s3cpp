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

    if (res.status() >= 200 && res.status() < 300) {
        return deserializeListBucketResult(XMLBody, maxKeys);
    }
    return std::unexpected<Error>(deserializeError(XMLBody));
}

std::expected<ListObjectsResult, Error> S3Client::deserializeListBucketResult(const std::vector<XMLNode>& nodes, const int maxKeys) {
    ListObjectsResult response;
    response.Contents.reserve(maxKeys);
    response.CommonPrefixes.reserve(maxKeys);

    response.Contents.push_back(Contents_ {});
    response.CommonPrefixes.push_back(CommonPrefix {});

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
                response.Contents.push_back(Contents_ {});
                seenContents.clear();
                contentsIdx++;
            }
        } else if (node.tag.contains("ListBucketResult.CommonPrefix")) {
            if (std::find(seenCommonPrefix.begin(), seenCommonPrefix.end(), node.tag) != seenCommonPrefix.end()) {
                response.CommonPrefixes.push_back(CommonPrefix {});
                seenCommonPrefix.clear();
                commonPrefixesIdx++;
            }
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
        } else if (node.tag == "ListBucketResult.KeyCount") {
            response.KeyCount = Parser.parseNumber<int>(std::move(node.value));
        } else if (node.tag == "ListBucketResult.ContinuationToken") {
            response.ContinuationToken = std::move(node.value);
        } else if (node.tag == "ListBucketResult.NextContinuationToken") {
            response.NextContinuationToken = std::move(node.value);
        } else if (node.tag == "ListBucketResult.StartAfter") {
            response.StartAfter = std::move(node.value);
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
        } else if (node.tag == "ListBucketResult.CommonPrefixes.Prefix") {
            response.CommonPrefixes[commonPrefixesIdx].Prefix = std::move(node.value);
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
    if (!response.Contents.empty() && response.Contents[0].Key.empty()) {
        response.Contents.erase(response.Contents.begin());
    }
    if (!response.CommonPrefixes.empty() && response.CommonPrefixes[0].Prefix.empty()) {
        response.CommonPrefixes.erase(response.CommonPrefixes.begin());
    }

    return response;
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

    const std::vector<XMLNode>& XMLBody = Parser.parse(res.body());

    if (res.status() >= 200 && res.status() < 300) {
        return res.body();
    }
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
