cmake_minimum_required(VERSION 3.16)
project(spiffe-cpp)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

add_library(spiffe STATIC src/status.cpp src/grpc_client.cpp src/http2_client.cpp src/spiffe.cpp)

target_link_libraries(spiffe PRIVATE ${CURL_LIBRARIES} -lpthread -ldl)
target_include_directories(spiffe PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CURL_INCLUDE_DIRS})

target_include_directories(spiffe PRIVATE third_party/SimpleProtos)

# test binary
add_executable(spiffe_test test/main.cpp)
target_link_libraries(spiffe_test PRIVATE spiffe)
target_include_directories(spiffe_test PRIVATE spiffe)
