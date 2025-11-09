#ifndef S3CPP_HTTPCLIENT
#define S3CPP_HTTPCLIENT

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

  [[nodiscard]] HttpRequest &header(const std::string &&header,
                                    const std::string &value) {
    headers_[header] = value;
    return *this;
  }
  [[nodiscard]] HttpRequest &timeout(unsigned int seconds) {
    timeout_ = seconds;
    return *this;
  }

  HttpResponse execute();

  const std::string &getURL() const { return URL_; }
  const int getTimeout() const { return timeout_; }

private:
  HttpClient &client_;
  std::string URL_;
  std::unordered_map<std::string, std::string> headers_;
  int timeout_; // TODO(cristian): `std::chrono` ?
};

// HttpClient should only focus on handling the cURL handle
// and making the request (HttpRequest) and returning HttpResponse
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

  [[nodiscard]] HttpRequest get(const std::string &URL) {
    return HttpRequest{*this, URL};
  };

  HttpResponse execute(HttpRequest &request);

private:
  CURL *curl_handle;
  static size_t write_callback(char *ptr, size_t size, size_t nmemb,
                               void *userdata);
};

#endif
