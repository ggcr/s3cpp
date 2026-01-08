#include <s3cpp/s3.h>

int main() {
    S3Client client("minio_access", "minio_secret");
    ListObjectsPaginator paginator(client, "my-bucket", "path/to/", 100);

    int totalObjects = 0;

    while (paginator.HasMorePages()) {
        std::expected<ListObjectsResult, Error> page = paginator.NextPage();

				if (!page) {
            std::println("Error: {}", page.error().Message);
            return 1;
        }

        totalObjects += page->KeyCount;

        for (const auto& obj : page->Contents) {
            std::println("Key: {}", obj.Key);
        }
    }
    return 0;
}
