#include "gtest/gtest.h"
#include <s3cpp/s3.h>
#include <random>
#include <string>


class SetupMinIO : public ::testing::Test {
	// Setup a MinIO bucket with some contents already
	protected:
		static void SetUpTestSuite() {
			return;
		}
};

std::string generateRandomBucketName(const std::string& prefix = "test-bucket") {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 35);

    const char* chars = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::string suffix;
    for (int i = 0; i < 8; ++i) {
        suffix += chars[dis(gen)];
    }
    return prefix + "-" + suffix;
}

TEST(S3, ListObjectsBucket) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        // Assuming the bucket has the 10K objects
        // Once we implement PutObject we will do this ourselves with s3cpp
        std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket");
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

TEST(S3, ListObjectsBucketNotExists) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        std::expected<ListObjectsResult, Error> res = client.ListObjects("Does-not-exist");
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
        std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket", { .Prefix = "path/to/file_1.txt" });
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
        std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket", { .MaxKeys = 100, .Prefix = "path/to/" });
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
        std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket", { .MaxKeys = 1, .Prefix = "path/to/" });
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
        std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket", { .MaxKeys = 2, .Prefix = "path/to/" });
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
        std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket", { .Prefix = "path/to/" });
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
            std::expected<ListObjectsResult, Error> page = paginator.NextPage();
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

TEST(S3, GetObjectNotExists) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        auto response = client.GetObject("my-bucket", "does/not/exists.txt");
        if (response) { // should trigger error
            GTEST_FAIL();
        }
        EXPECT_EQ(response.error().Code, "NoSuchKey");
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping GetObjectNotExists: Server not up");
        }
        throw;
    }
}

TEST(S3, GetObjectBadBucket) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        auto response = client.GetObject("does-not-exist", "path/to/file_1.txt");
        if (response) { // should trigger error
            GTEST_FAIL();
        }
        EXPECT_EQ(response.error().Code, "NoSuchBucket");
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping GetObjectBadBucket: Server not up");
        }
        throw;
    }
}

TEST(S3, ListObjectsWithDelimiter) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket", { .Delimiter = "/", .Prefix = "path/" });
        if (!res) {
            GTEST_FAIL();
        }

        // With delimiter, we should get CommonPrefixes for subdirectories
        EXPECT_FALSE(res->CommonPrefixes.empty());
        EXPECT_EQ(res->Delimiter, "/");
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping ListObjectsWithDelimiter: Server not up");
        }
        throw;
    }
}

TEST(S3, ListObjectsWithStartAfter) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket", { .MaxKeys = 10, .Prefix = "path/to/", .StartAfter = "path/to/file_50.txt" });
        if (!res) {
            GTEST_FAIL();
        }

        // First key should be file_50.txt
        EXPECT_FALSE(res->Contents.empty());
        EXPECT_GT(res->Contents[0].Key, "path/to/file_50.txt");
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping ListObjectsWithStartAfter: Server not up");
        }
        throw;
    }
}

TEST(S3, ListObjectsWithContinuationToken) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        std::expected<ListObjectsResult, Error> firstPage = client.ListObjects("my-bucket", { .MaxKeys = 10, .Prefix = "path/to/" });
        if (!firstPage) {
            GTEST_FAIL();
        }

        EXPECT_TRUE(firstPage->IsTruncated);
        EXPECT_FALSE(firstPage->NextContinuationToken.empty());

        // using continuation token
        std::expected<ListObjectsResult, Error> secondPage = client.ListObjects("my-bucket", { .ContinuationToken = firstPage->NextContinuationToken, .MaxKeys = 10, .Prefix = "path/to/" });
        if (!secondPage)
            GTEST_FAIL();

        EXPECT_FALSE(secondPage->Contents.empty());
        EXPECT_NE(firstPage->Contents[0].Key, secondPage->Contents[0].Key);
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping ListObjectsWithContinuationToken: Server not up");
        }
        throw;
    }
}

TEST(S3, GetObjectWithRange) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        // first 10 bytes
        auto response = client.GetObject("my-bucket", "path/to/file_1.txt", { .Range = "bytes=0-3" });
        if (!response) {
            GTEST_FAIL();
        }

        EXPECT_EQ(response.value().size(), 4);
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping GetObjectWithRange: Server not up");
        }
        throw;
    }
}

TEST(S3, PutObjectTxt) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        auto PutResponse = client.PutObject("my-bucket", "some/file.txt", "hello, from s3");
        if (!PutResponse) {
            FAIL() << std::format("PutObject request failed: Code={}, Message={}",
                PutResponse.error().Code,
                PutResponse.error().Message);
        }
        std::expected<std::string, Error> GetResponse = client.GetObject("my-bucket", "some/file.txt");
        if (!GetResponse) {
            FAIL() << std::format("GetObject request failed: Code={}, Message={}",
                GetResponse.error().Code,
                GetResponse.error().Message);
        }
        EXPECT_EQ(GetResponse.value(), "hello, from s3");
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error for request: Could not connect to server") {
            GTEST_SKIP_("Skipping GetObjectWithRange: Server not up");
        }
        throw;
    }
}

TEST(S3, CreateBucket) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        std::string bucketName = generateRandomBucketName("test-bucket-s3cpp");
        CreateBucketConfiguration config;
        CreateBucketInput options;

        auto res = client.CreateBucket(bucketName, config, options);
        if (!res) {
            FAIL() << std::format("CreateBucket request failed: Code={}, Message={}",
                res.error().Code,
                res.error().Message);
        }

        std::expected<ListObjectsResult, Error> listRes = client.ListObjects(bucketName);
        if (!listRes) {
            FAIL() << std::format("ListObjects after CreateBucket failed: Code={}, Message={}",
                listRes.error().Code,
                listRes.error().Message);
        }
        EXPECT_EQ(listRes->Name, bucketName);
        EXPECT_EQ(listRes->Contents.size(), 0);
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error for request: Could not connect to server") {
            GTEST_SKIP_("Skipping CreateBucket: Server not up");
        }
        throw;
    }
}

TEST(S3, CreateBucketWithLocationConstraint) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        std::string bucketName = generateRandomBucketName("test-bucket-location");
        CreateBucketConfiguration config;
        config.LocationConstraint = "us-west-2";
        CreateBucketInput options;

        auto res = client.CreateBucket(bucketName, config, options);
        if (!res) {
            FAIL() << std::format("CreateBucket with LocationConstraint failed: Code={}, Message={}",
                res.error().Code,
                res.error().Message);
        }

        EXPECT_FALSE(res->Location.empty());
        EXPECT_EQ(res->Location, std::format("/{}", bucketName));

        std::expected<ListObjectsResult, Error> listRes = client.ListObjects(bucketName);
        if (!listRes) {
            FAIL() << "Failed to list objects in newly created bucket";
        }
        EXPECT_EQ(listRes->Name, bucketName);
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error for request: Could not connect to server") {
            GTEST_SKIP_("Skipping CreateBucketWithLocationConstraint: Server not up");
        }
        throw;
    }
}

TEST(S3, CreateBucketWithTags) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        std::string bucketName = generateRandomBucketName("test-bucket-tags");
        CreateBucketConfiguration config;
        config.Tags = {
            {"Environment", "Test"},
            {"Project", "s3cpp"}
        };
        CreateBucketInput options;

        auto res = client.CreateBucket(bucketName, config, options);
        if (!res) {
            FAIL() << std::format("CreateBucket with Tags failed: Code={}, Message={}",
                res.error().Code,
                res.error().Message);
        }

        std::expected<ListObjectsResult, Error> listRes = client.ListObjects(bucketName);
        if (!listRes) {
            FAIL() << "Failed to list objects in newly created bucket";
        }
        EXPECT_EQ(listRes->Name, bucketName);
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error for request: Could not connect to server") {
            GTEST_SKIP_("Skipping CreateBucketWithTags: Server not up");
        }
        throw;
    }
}

TEST(S3, CreateBucketAlreadyExists) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    try {
        CreateBucketConfiguration config;
        CreateBucketInput options;

        auto res = client.CreateBucket("my-bucket", config, options);
        if (res) {
            FAIL() << "CreateBucket should fail when bucket already exists";
        }

        // BucketAlreadyOwnedByYou error
        Error error = res.error();
        EXPECT_TRUE(error.Code == "BucketAlreadyOwnedByYou" || error.Code == "BucketAlreadyExists");
    } catch (const std::exception& e) {
        const std::string emsg = e.what();
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error for request: Could not connect to server") {
            GTEST_SKIP_("Skipping CreateBucketAlreadyExists: Server not up");
        }
        throw;
    }
}
