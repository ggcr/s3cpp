#ifndef S3CPP_HTTPCLIENT
#define S3CPP_HTTPCLIENT

#include <curl/curl.h>
#include <curl/easy.h>
#include <stdexcept>
#include <string>

class HttpResponse {
public:
  HttpResponse(int c, std::string b) : code_(c), body_(std::move(b)) {};

  [[nodiscard]] int status() const { return code_; }
  const std::string &body() const { return body_; }

  bool is_ok() const { return code_ >= 200 && code_ < 300; }
  bool is_client_error() const { return code_ >= 400 && code_ < 500; }
  bool is_server_error() const { return code_ >= 500 && code_ < 600; }

private:
  int code_;
  std::string body_;
};

class HttpClient {
public:
  HttpClient() {
    curl_handle = curl_easy_init();
    if (!curl_handle)
      throw std::runtime_error("Failed to initialize cURL");
  }
  ~HttpClient() {
    if (curl_handle)
      curl_easy_cleanup(curl_handle);
  };

  HttpClient(const HttpClient &) = delete;
  HttpClient &operator=(const HttpClient &) = delete;

  // When transfering ownership we must invalidate the source cURL handle
  HttpClient(HttpClient &&other) : curl_handle(other.curl_handle) {
    other.curl_handle = nullptr;
  }
  HttpClient &operator=(HttpClient &&other) {
    if (this != &other) {
      // cleanup current handle
      if (curl_handle)
        curl_easy_cleanup(curl_handle);
      curl_handle = std::move(other.curl_handle);
      other.curl_handle = nullptr;
    }
    return *this;
  }

  HttpResponse get(const std::string &URL);

private:
  CURL *curl_handle;
  static size_t write_callback(char *ptr, size_t size, size_t nmemb,
                               void *userdata);
};

#endif
