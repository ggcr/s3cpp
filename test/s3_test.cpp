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

TEST(S3, ListObjectsDirPrefixMaxKeys) {
    S3Client client("minio_access", "minio_secret");
    try {
        client.list_objects("my-bucket", "path/to/", 1);
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping MinIOBasicRequest: Server not up");
        }
        throw;
    }
}

TEST(S3, ListObjectsCheckFields) {
    S3Client client("minio_access", "minio_secret");
    try {
        ListBucketResult response = client.list_objects("my-bucket", "path/to/", 2);

        // Check top-level fields
        EXPECT_EQ(response.Name, "my-bucket");
        EXPECT_EQ(response.Prefix, "path/to/");
        EXPECT_EQ(response.MaxKeys, 2);
        EXPECT_EQ(response.IsTruncated, true);
        EXPECT_FALSE(response.NextMarker.empty());

        // Should have exactly 2 contents
        EXPECT_EQ(response.Contents.size(), 2);

        // Check first object
        EXPECT_EQ(response.Contents[0].Key, "path/to/file_1.txt");
        EXPECT_EQ(response.Contents[0].Size, 26);
        EXPECT_EQ(response.Contents[0].Owner.ID, "02d6176db174dc93cb1b899f7c6078f08654445fe8cf1b6ce98d8855f66bdbf4");
        EXPECT_EQ(response.Contents[0].Owner.DisplayName, "minio");
        EXPECT_EQ(response.Contents[0].StorageClass, "STANDARD");

        // Check second object
        EXPECT_EQ(response.Contents[1].Key, "path/to/file_10.txt");
        EXPECT_EQ(response.Contents[1].Size, 27);
        EXPECT_EQ(response.Contents[1].Owner.ID, "02d6176db174dc93cb1b899f7c6078f08654445fe8cf1b6ce98d8855f66bdbf4");
        EXPECT_EQ(response.Contents[1].Owner.DisplayName, "minio");
        EXPECT_EQ(response.Contents[1].StorageClass, "STANDARD");
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping MinIOBasicRequest: Server not up");
        }
        throw;
    }
}

TEST(S3, ListObjectsCheckLenKeys) {
    S3Client client("minio_access", "minio_secret");
    try {
				// has 10K objects - limit is 1000 keys
        ListBucketResult response = client.list_objects("my-bucket", "path/to/");
				EXPECT_EQ(response.Contents.size(), 1000);
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping MinIOBasicRequest: Server not up");
        }
        throw;
    }
}
