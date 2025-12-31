#include "gtest/gtest.h"
#include <s3cpp/s3.h>

TEST(S3, ListObjectsBucket) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        // Assuming the bucket has the 10K objects
        // Once we implement PutObject we will do this ourselves with s3cpp
        std::expected<ListBucketResult, Error> res = client.ListObjects("my-bucket");
        if (!res)
            GTEST_FAIL();
        EXPECT_EQ(res->Contents.size(), 0);
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping MinIOBasicRequest: Server not up");
        }
        throw;
    }
}

TEST(S3, ListObjectsBucketNotExists) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        std::expected<ListBucketResult, Error> res = client.ListObjects("Does-not-exist");
        if (res.has_value()) // We must return error
            GTEST_FAIL();
				Error error = res.error();
        // EXPECT_EQ(error.Code, "InvalidBucketName");
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping MinIOBasicRequest: Server not up");
        }
        throw;
    }
}

TEST(S3, ListObjectsFilePrefix) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        // path/to/file_1.txt must exist
        std::expected<ListBucketResult, Error> res = client.ListObjects("my-bucket", "path/to/file_1.txt");
        if (!res)
            GTEST_FAIL();
        EXPECT_EQ(res->Contents.size(), 1);
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping MinIOBasicRequest: Server not up");
        }
        throw;
    }
}

TEST(S3, ListObjectsDirPrefix) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        // Get 100 keys
        std::expected<ListBucketResult, Error> res = client.ListObjects("my-bucket", "path/to/", 100);
        if (!res)
            GTEST_FAIL();
        EXPECT_EQ(res->Contents.size(), 100);
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping MinIOBasicRequest: Server not up");
        }
        throw;
    }
}

TEST(S3, ListObjectsDirPrefixMaxKeys) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        std::expected<ListBucketResult, Error> res = client.ListObjects("my-bucket", "path/to/", 1);
        if (!res)
            GTEST_FAIL();
        EXPECT_EQ(res->Contents.size(), 1);
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping MinIOBasicRequest: Server not up");
        }
        throw;
    }
}

TEST(S3, ListObjectsCheckFields) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        std::expected<ListBucketResult, Error> res = client.ListObjects("my-bucket", "path/to/", 2);
        if (!res)
            GTEST_FAIL();

        EXPECT_EQ(res->Name, "my-bucket");
        EXPECT_EQ(res->Prefix, "path/to/");
        EXPECT_EQ(res->MaxKeys, 2);
        EXPECT_EQ(res->IsTruncated, true);
        EXPECT_FALSE(res->NextContinuationToken.empty());

        // Should have exactly 2 keys
        EXPECT_EQ(res->Contents.size(), 2);

        EXPECT_EQ(res->Contents[0].Key, "path/to/file_1.txt");
        EXPECT_EQ(res->Contents[0].Size, 26);
        EXPECT_EQ(res->Contents[0].StorageClass, "STANDARD");

        EXPECT_EQ(res->Contents[1].Key, "path/to/file_10.txt");
        EXPECT_EQ(res->Contents[1].Size, 27);
        EXPECT_EQ(res->Contents[1].StorageClass, "STANDARD");

    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping MinIOBasicRequest: Server not up");
        }
        throw;
    }
}

TEST(S3, ListObjectsCheckLenKeys) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        // has 10K objects - limit is 1000 keys
        std::expected<ListBucketResult, Error> res = client.ListObjects("my-bucket", "path/to/");
        if (!res)
            GTEST_FAIL();
        EXPECT_EQ(res->Contents.size(), 1000);
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping MinIOBasicRequest: Server not up");
        }
        throw;
    }
}

TEST(S3, ListObjectsPaginator) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        // has 10K objects - fetch 100 per page
        ListObjectsPaginator paginator(client, "my-bucket", "path/to/", 100);

        int totalObjects = 0;
        int pageCount = 0;

        while (paginator.HasMorePages()) {
            std::expected<ListBucketResult, Error> page = paginator.NextPage();
            if (!page) {
                GTEST_FAIL();
            }
            totalObjects += page->Contents.size();
            if (page->Contents.size() > 0)
                pageCount++;

            if (paginator.HasMorePages()) {
                EXPECT_EQ(page->Contents.size(), 100);
                EXPECT_TRUE(page->IsTruncated);
            }
        }

        EXPECT_EQ(totalObjects, 10000);
        EXPECT_EQ(pageCount, 100);
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping ListObjectsPaginator: Server not up");
        }
        throw;
    }
}

TEST(S3, GetObjectExists) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        auto response = client.GetObject("my-bucket", "path/to/file_1.txt");
        if (!response) {
            GTEST_FAIL();
        }
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping GetObjectExists: Server not up");
        }
        throw;
    }
}
