#include <curl/curl.h>
#include <curl/easy.h>
#include <format>
#include <s3cpp/httpclient.h>
#include <stdexcept>
#include <string>

HttpResponse HttpRequest::execute() { return client_.execute(*this); }


HttpResponse HttpClient::execute(HttpRequest &request) {
  if (!curl_handle) {
    throw std::runtime_error(
        "cURL handle has been invalidated in favour of another client");
  }
  std::string buffer;
  std::string error_buf;

  // TODO(cristian): from libcurl docs, they state that each curl handle has
  // "sticky" params, this is why we are resetting at each get request
  // However, I think we should only do this when moving a new handle the only
  // thing that will change is the URL from now
  //
  // curl_easy_reset(curl_handle);

  curl_easy_setopt(curl_handle, CURLOPT_URL, request.getURL().c_str());
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &buffer);
  curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, request.getTimeout());

  CURLcode code = curl_easy_perform(curl_handle);
  if (code != CURLE_OK) {
    throw std::runtime_error(
        std::format("libcurl error for request: {}", curl_easy_strerror(code)));
  }

  // get HTTP code
  long response_code = 0;
  curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &response_code);

  return HttpResponse(static_cast<int>(response_code), std::move(buffer));
}

size_t HttpClient::write_callback(char *ptr, size_t size, size_t nmemb,
                                  void *userdata) {
  std::string *buffer = static_cast<std::string *>(userdata);
  size_t total_size = size * nmemb;
  buffer->append(ptr, total_size);
  return total_size;
}
