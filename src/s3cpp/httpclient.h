#ifndef S3CPP_HTTPCLIENT
#define S3CPP_HTTPCLIENT

#include <chrono>
#include <cstddef>
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdexcept>
#include <string>
#include <unordered_map>

// Forward declaration
class HttpClient;

class HttpResponse {
public:
  HttpResponse(int c, std::string b) : code_(c), body_(std::move(b)) {};

  int status() const { return code_; }
  const std::string &body() const { return body_; }

  bool is_ok() const { return code_ >= 200 && code_ < 300; }
  bool is_redirect() const { return code_ >= 300 && code_ < 400; }
  bool is_client_error() const { return code_ >= 400 && code_ < 500; }
  bool is_server_error() const { return code_ >= 500 && code_ < 600; }

  bool operator==(const HttpResponse &other) const {
    return (code_ == other.code_ && body_ == other.body_);
  }

private:
  int code_;
  std::string body_;
};

// HttpRequest will handle all the headers and request params
class HttpRequest {
public:
  HttpRequest(HttpClient &client, std::string URL)
      : client_(client), URL_(std::move(URL)), timeout_(0) {};

  HttpRequest &timeout(const long long &seconds) {
    timeout_ = std::chrono::seconds(seconds);
    return *this;
  }
  HttpRequest &timeout(const std::chrono::seconds &seconds) {
    timeout_ = seconds;
    return *this;
  }
  HttpRequest &header(const std::string &header_, const std::string &value) {
    headers_[header_] = value;
    return *this;
  }

  HttpResponse execute();

  const std::string &getURL() const { return URL_; }
  const long long getTimeout() const { return timeout_.count(); }
  const std::unordered_map<std::string, std::string>  &getHeaders() const { return headers_; }

private:
  HttpClient &client_;
  std::string URL_;
  std::unordered_map<std::string, std::string> headers_;
  std::chrono::seconds timeout_;
};

// HttpClient should only focus on handling the cURL handle
// and making the request (HttpRequest) and returning HttpResponse
class HttpClient {
  friend class HttpRequest; // `execute()` is invoked from the request only
public:
  HttpClient() {
    curl_handle = curl_easy_init();
    if (!curl_handle)
      throw std::runtime_error("Failed to initialize cURL");
		headers_["User-Agent"] = "s3cpp/0.0.0 github.com/ggcr/s3cpp";
  }
  ~HttpClient() {
    if (curl_handle)
      curl_easy_cleanup(curl_handle);
  };
  HttpClient(std::unordered_map<std::string, std::string> headers)
      : headers_(std::move(headers)) {
    curl_handle = curl_easy_init();
    if (!curl_handle)
      throw std::runtime_error("Failed to initialize cURL");
		headers_["User-Agent"] = "s3cpp/0.0.0 github.com/ggcr/s3cpp";
  }

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

  // HTTP GET
  [[nodiscard]] HttpRequest get(const std::string &URL) {
    return HttpRequest{*this, URL};
  };

private:
  CURL *curl_handle = nullptr;
  static size_t write_callback(char *ptr, size_t size, size_t nmemb,
                               void *userdata);
  std::unordered_map<std::string, std::string> headers_;

  // main logic to perform the request
  // this is invoked by HttpRequest
  HttpResponse execute(HttpRequest &request);

  const std::unordered_map<std::string, std::string> &getHeaders() const { return headers_; }
};

#endif
