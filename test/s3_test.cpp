#include "gtest/gtest.h"
#include <s3cpp/s3.hpp>

// #include "S3Client.h"
//
// int main() {
//     S3Client client("access_key", "secret_key", "us-east-1");
//
//     // Upload object
//     client.put_object("my-bucket", "path/to/file.txt", "Hello, S3!");
//
//     // Download object
//     std::string content = client.get_object("my-bucket", "path/to/file.txt");
//
//     // List objects
//     auto objects = client.list_objects("my-bucket", "path/");
//     for (const auto& obj : objects) {
//         std::cout << obj.key << " - " << obj.size << " bytes\n";
//     }
//
//     // Delete object
//     client.delete_object("my-bucket", "path/to/file.txt");
//
//     return 0;
// }

TEST(S3, Hello) {
    S3Client client("minio_access", "minio_secret");
}
