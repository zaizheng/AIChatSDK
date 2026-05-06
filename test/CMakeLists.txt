cmake_minimum_required(VERSION 3.10)

project(testLLM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

add_executable(testLLM testLLM.cpp
../sdk/src/util/myLog.cpp
../sdk/src/DeepSeekProvider.cpp
../sdk/src/ChatGPTProvider.cpp)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

include_directories(${PROJECT_INCLUDE_DIR}/../sdk/include)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

target_compile_definitions(testLLM PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

target_link_libraries(testLLM jsoncpp fmt spdlog gtest OpenSSL::SSL OpenSSL::Crypto)
 