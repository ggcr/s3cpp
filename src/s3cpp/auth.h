#ifndef S3CPP_AUTH
#define S3CPP_AUTH

#include "s3cpp/httpclient.h"
#include <cstdint>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <string>

class AWSSigV4Signer {
public:
    AWSSigV4Signer(const std::string& access, const std::string& secret)
        : access_key(std::move(access))
        , secret_key(std::move(secret))
        , aws_region("us-east-2") { }
    AWSSigV4Signer(const std::string& access,
        const std::string& secret,
        const std::string& region)
        : access_key(std::move(access))
        , aws_region(std::move(region))
        , secret_key(std::move(secret)) { }

    void sign(HttpRequest& request) {
        // Autorization
        const std::string hash_algo = "AWS4-HMAC-SHA256";

        // Compute date
        const auto req_headers = request.getHeaders();
        const std::string timestamp = req_headers.at("X-Amz-Date");
        const std::string request_date = timestamp.substr(0, 8);

        // Credential
        const std::string credential_scope = std::format("{}/{}/s3/aws4_request", request_date, aws_region);

        // Signed headers
        std::string signed_headers = "";
        uint_fast16_t i = 0;
        for (const auto& header : req_headers) {
            std::string kHeader = header.first;
            std::transform(kHeader.begin(), kHeader.end(), kHeader.begin(),
                ::tolower);
            if (i > 0)
                signed_headers += ";";
            signed_headers += kHeader;
            i++;
        }

        // Cannonical request
        std::string hex_cannonical_request = hex(sha256(createCannonicalRequest(request)));

        // To sign
        std::string string_to_sign = std::format("{}\n{}\n{}\n{}", hash_algo, timestamp, credential_scope,
            hex_cannonical_request);
        std::string signature = hex(HMAC_SHA256(
            deriveSigningKey(request_date), SHA256_DIGEST_LENGTH, string_to_sign));

        // Build the final auth header value
        request.header(
            "Authorization",
            std::format("{} Credential={}/{}, SignedHeaders={}, Signature={}",
                hash_algo, access_key, credential_scope, signed_headers,
                signature));
    }

    std::string createCannonicalRequest(HttpRequest& request) {
        const std::string http_verb = getHttpVerb(request.getHttpMethod());

        // URI
        std::string url = request.getURL();
        std::string uri {};
        if (size_t bpos = url.find("amazonaws.com"); bpos != std::string::npos) {
            uri = url.erase(0, bpos + 13);
        } else {
            // Assume localhost:XXXX (dirty, sorry :( i know)
            size_t path_start = url.find('/', 7);
            if (path_start != std::string::npos) {
                uri = url.substr(path_start);
            } else {
                uri = "/";
            }
        }

        // URI Query-string
        // For now let's assume the request does not include any query string...
        if (size_t epos = url.find("?"); epos != std::string::npos) {
            uri = url.erase(epos, uri.size());
        }
        std::string query_str = "";

        // Canonical Headers + SignedHeaders
        const std::map<std::string, std::string> headers = request.getHeaders();
        std::string cheaders = "";
        std::string signed_headers = "";
        uint_fast16_t i = 0;
        for (const auto& header : headers) {
            std::string kHeader = header.first;
            std::transform(kHeader.begin(), kHeader.end(), kHeader.begin(),
                ::tolower);
            if (i > 0)
                signed_headers += ";";
            signed_headers += kHeader;
            cheaders += kHeader + ":" + header.second + "\n";
            i++;
        }

        // Hashed payload (assume empty for now)
        const std::string empty_payload_hash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

        return std::format("{}\n{}\n{}\n{}\n{}\n{}", http_verb, uri, query_str,
            cheaders, signed_headers, empty_payload_hash);
    }

    const unsigned char* sha256(const std::string& str) {
        // Returns the SHA256 digest in Hex
        const auto in_str = reinterpret_cast<const unsigned char*>(str.c_str());
        const unsigned char* digest = SHA256(in_str, str.size(), NULL);
        return digest;
    }

    const unsigned char* HMAC_SHA256(const unsigned char* key,
        size_t key_len,
        const std::string& data) {
        return HMAC(EVP_sha256(), key, key_len,
            reinterpret_cast<const unsigned char*>(data.c_str()),
            data.size(), NULL, NULL);
    }

    std::string hex(const unsigned char* hash) {
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int>(hash[i]);
        }
        return ss.str();
    }

    // Move to S3 client class
    // --
    // Required by AWS SigV4 to be in ISO8601 format
    // https://docs.aws.amazon.com/IAM/latest/UserGuide/reference_sigv-create-signed-request.html
    std::string getTimestamp() {
        const std::chrono::time_point now { std::chrono::system_clock::now() };
        return std::format("{:%Y%m%dT%H%M%SZ}",
            std::chrono::floor<std::chrono::seconds>(now));
    }

private:
    std::string access_key;
    std::string secret_key;
    std::string aws_region;

    // Cannonicalize HTTP verb from the request
    std::string getHttpVerb(const HttpMethod& http_method) {
        switch (http_method) {
        case HttpMethod::Get:
            return "GET";
        case HttpMethod::Head:
            return "HEAD";
        case HttpMethod::Post:
            return "POST";
        case HttpMethod::Put:
            return "PUT";
        case HttpMethod::Delete:
            return "DELETE";
        default:
            throw std::runtime_error("No known Http Method");
        }
    }

    const unsigned char* deriveSigningKey(const std::string request_date) {
        const std::string initial_candidate = "AWS4" + secret_key;
        const unsigned char* keyCandidate = reinterpret_cast<const unsigned char*>(initial_candidate.c_str());
        const unsigned char* DateKey = HMAC_SHA256(keyCandidate, initial_candidate.size(), request_date);
        const unsigned char* DateRegionKey = HMAC_SHA256(DateKey, SHA256_DIGEST_LENGTH, aws_region);
        const unsigned char* DateRegionServiceKey = HMAC_SHA256(DateRegionKey, SHA256_DIGEST_LENGTH, "s3");
        const unsigned char* SigningKey = HMAC_SHA256(DateRegionServiceKey, SHA256_DIGEST_LENGTH, "aws4_request");
        return SigningKey;
    }
};

#endif
