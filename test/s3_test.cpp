#include "gtest/gtest.h"
#include <s3cpp/s3.hpp>

TEST(S3, ListObjectsNoPrefix) {
    S3Client client("minio_access", "minio_secret");
    try {
        client.list_objects("my-bucket");
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping MinIOBasicRequest: Server not up");
        }
        throw;
    }
}

TEST(S3, ListObjectsFilePrefix) {
    S3Client client("minio_access", "minio_secret");
    try {
        client.list_objects("my-bucket", "path/to/file.txt");
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping MinIOBasicRequest: Server not up");
        }
        throw;
    }
}

TEST(S3, ListObjectsDirPrefix) {
    S3Client client("minio_access", "minio_secret");
    try {
        client.list_objects("my-bucket", "path/to/");
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping MinIOBasicRequest: Server not up");
        }
        throw;
    }
}
