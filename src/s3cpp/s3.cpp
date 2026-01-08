#include <expected>
#include <s3cpp/s3.h>

std::expected<ListBucketResult, ErrorNoSuchBucket> S3Client::ListObjects(const std::string& bucket, const std::string& prefix, int maxKeys, const std::string& continuationToken) {
    // Silent-ly accept maxKeys > 1000, even though we will return 1K at most
    // Pagination is opt-in as in the Go SDK, the user must be aware of this
    const std::string baseUrl = buildURL(bucket);
    std::string url;
    if (continuationToken.size() > 0) {
        url = baseUrl + std::format("?list-type=2&prefix={}&max-keys={}&continuation-token={}", prefix, maxKeys, continuationToken);
    } else {
        url = baseUrl + std::format("?list-type=2&prefix={}&max-keys={}", prefix, maxKeys);
    }

    HttpRequest req = Client.get(url).header("Host", getHostHeader(bucket));
    Signer.sign(req);
    HttpResponse res = req.execute();

    const std::vector<XMLNode>& XMLBody = Parser.parse(res.body());
    std::expected<ListBucketResult, ErrorNoSuchBucket> response = deserializeListBucketResult(XMLBody, maxKeys);
    return response;
}

std::expected<ListBucketResult, ErrorNoSuchBucket> S3Client::deserializeListBucketResult(const std::vector<XMLNode>& nodes, const int maxKeys) {
    ListBucketResult response;
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
        } else {
            // Detect and parse error
            if (node.tag.substr(0, 6) == "Error.") {
                return std::unexpected<ErrorNoSuchBucket>(deserializeError(nodes));
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

ErrorNoSuchBucket S3Client::deserializeError(const std::vector<XMLNode>& nodes) {
    ErrorNoSuchBucket error;

    for (const auto& node : nodes) {
        /* Sigh... no reflection */

        if (node.tag == "Error.Code") {
            error.Code = std::move(node.value);
        } else if (node.tag == "Error.Message") {
            error.Message = std::move(node.value);
        } else if (node.tag == "Error.Resource") {
            error.Resource = std::move(node.value);
        } else if (node.tag == "Error.BucketName") {
            error.BucketName = std::move(node.value);
        } else if (node.tag == "Error.RequestId") {
            error.RequestId = Parser.parseNumber<int>(std::move(node.value));
        } else if (node.tag == "Error.HostId") {
            error.HostId = std::move(node.value);
        } else {
            throw std::runtime_error(std::format("No case for Error response found for: {}", node.tag));
        }
    }

    return error;
}
