# s3cpp

> [!WARNING]
> **WIP** Currently supports only ListObjectsV2

A lightweight C++ client library for AWS S3, with zero 3rd party C++ dependencies (only libcurl and OpenSSL). Inspired by the AWS SDK for Go.

## Architecture

Each S3 Client is organized onto modular components:

- `src/s3cpp/httpclient`: HTTP/1.1 client built on libCurl
- `src/s3cpp/auth`: AWS Signature V4 auth protocol (SigV4a pending)
- `src/s3cpp/xml`: A custom FSM for parsing XML

## Basic Usage

```cpp
#include <s3cpp/s3.hpp>

int main() {
    S3Client client("your_access_key", "your_secret_key");
    // List 100 objects with a prefix
    ListBucketResult response = client.ListObjects("my-bucket", "path/to/", 100);
    for (const auto& obj : response.Contents) {
        std::println("Key: {}, Size: {}", obj.Key, obj.Size);
    }
    return 0;
}
```

For buckets with many objects, use the paginator to automatically handle continuation tokens:

```cpp
#include <s3cpp/s3.hpp>

int main() {
    S3Client client("your_access_key", "your_secret_key");
    ListObjectsPaginator paginator(client, "my-bucket", "path/to/", 100);

    int totalObjects = 0;

    while (paginator.HasMorePages()) {
        ListBucketResult page = paginator.NextPage();
        totalObjects += page.KeyCount;

        for (const auto& obj : page.Contents) {
            std::println("Key: {}", obj.Key);
        }
    }
    return 0;
}
```

## Build and Test

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/tests
```

Some tests require a local MinIO container to be running:

```bash
docker run -p 9000:9000 -p 9001:9001 \
  -e "MINIO_ROOT_USER=minio_access" \
  -e "MINIO_ROOT_PASSWORD=minio_secret" \
  quay.io/minio/minio server /data --console-address ":9001"
```

The test suite contains 37 tests covering HTTP, AWS SigV4, XML, and S3 ListObjectsV2
