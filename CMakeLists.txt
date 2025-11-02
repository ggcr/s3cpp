cmake_minimum_required(VERSION 4.1.2)
project(s3cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 

# Fetch the gtest dependency
include(FetchContent)
FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/6ec14dfd8c409d05fba94e18e3a02df35b874353.zip
)
FetchContent_MakeAvailable(googletest)

# Library
add_library(s3cpplib src/s3cpp/foo.cpp)
target_include_directories(s3cpplib PUBLIC src)

# App
add_executable(s3cpp_app main.cpp)
target_link_libraries(s3cpp_app s3cpplib)

# Tests
enable_testing()
add_executable(s3cpp_tests test/foo_test.cpp)
target_link_libraries(s3cpp_tests s3cpplib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(s3cpp_tests)
