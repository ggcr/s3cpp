#include <curl/curl.h>
#include <curl/easy.h>
#include <format>
#include <s3cpp/httpclient.h>
#include <stdexcept>
#include <string>

// Route to its HttpMethod
HttpResponse HttpRequest::execute() { 
	switch (this->http_method_) {
		case HttpMethod::Get:
			return client_.execute_get(*this); 
		case HttpMethod::Head:
			return client_.execute_head(*this); 
		default:
			throw std::runtime_error(std::format("No matching enum Http Method"));
	}
}

HttpResponse HttpClient::execute_get(HttpRequest &request) {
  if (!curl_handle) {
    throw std::runtime_error(
        // this can happen both when cURL handle is not initialized or when it
        // is invalidated in the HTTPClient copy constructor
        "cURL handle is invalid");
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

	// merge client and request headers
	// https://stackoverflow.com/questions/34321719
	auto headers = request.getHeaders();
	headers.insert(this->getHeaders().begin(), this->getHeaders().end());
	struct curl_slist *list = NULL;
	for (const auto &[k, v] : headers) {
		list = curl_slist_append(list, std::format("{}: {}", k, v).c_str());
	}
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);

	CURLcode code = curl_easy_perform(curl_handle);
	curl_slist_free_all(list);
  if (code != CURLE_OK) {
    throw std::runtime_error(
        std::format("libcurl error for request: {}", curl_easy_strerror(code)));
  }

  // get HTTP code
  long response_code = 0;
  curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &response_code);

  return HttpResponse(static_cast<int>(response_code), std::move(buffer));
}

HttpResponse HttpClient::execute_head(HttpRequest &request) {
  if (!curl_handle) {
    throw std::runtime_error(
        // this can happen both when cURL handle is not initialized or when it
        // is invalidated in the HTTPClient copy constructor
        "cURL handle is invalid");
  }
  std::string error_buf;

  // TODO(cristian): from libcurl docs, they state that each curl handle has
  // "sticky" params, this is why we are resetting at each get request
  // However, I think we should only do this when moving a new handle the only
  // thing that will change is the URL from now
  //
  // curl_easy_reset(curl_handle);

  curl_easy_setopt(curl_handle, CURLOPT_URL, request.getURL().c_str());
  curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);
  curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, request.getTimeout());

	// merge client and request headers
	// https://stackoverflow.com/questions/34321719
	auto headers = request.getHeaders();
	headers.insert(this->getHeaders().begin(), this->getHeaders().end());
	struct curl_slist *list = NULL;
	for (const auto &[k, v] : headers) {
		list = curl_slist_append(list, std::format("{}: {}", k, v).c_str());
	}
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);

	CURLcode code = curl_easy_perform(curl_handle);
	curl_slist_free_all(list);
  if (code != CURLE_OK) {
    throw std::runtime_error(
        std::format("libcurl error for request: {}", curl_easy_strerror(code)));
  }

  // HTTP code
  long response_code = 0;
  curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &response_code);

  return HttpResponse(static_cast<int>(response_code));
}

size_t HttpClient::write_callback(char *ptr, size_t size, size_t nmemb,
                                  void *userdata) {
  std::string *buffer = static_cast<std::string *>(userdata);
  size_t total_size = size * nmemb;
  buffer->append(ptr, total_size);
  return total_size;
}
