cmake_minimum_required(VERSION 3.31.6)
project(s3cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 

# GoogleTest
include(FetchContent)
FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/6ec14dfd8c409d05fba94e18e3a02df35b874353.zip
)
FetchContent_MakeAvailable(googletest)

# cURL
find_package(CURL)

# Create lib and app
add_library(s3cpplib 
	src/s3cpp/httpclient.cpp
)
target_include_directories(s3cpplib PUBLIC src)
add_executable(s3cpp_app main.cpp)
target_link_libraries(s3cpp_app s3cpplib)

# Tests
enable_testing()
add_executable(tests 
	test/httpclient_test.cpp
)

target_link_libraries(tests s3cpplib GTest::gtest_main GTest::gmock_main CURL::libcurl)

include(GoogleTest)
gtest_discover_tests(tests)
