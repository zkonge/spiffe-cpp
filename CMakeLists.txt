cmake_minimum_required(VERSION 3.16)
project(spiffe-cpp)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sanitizer options
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_FUZZING "Enable Fuzzing" OFF)

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()

find_package(CURL REQUIRED)

# SPIFFE Library
add_library(spiffe SHARED src/status.cpp src/der.cpp src/grpc_client.cpp src/http2_client.cpp src/spiffe.cpp)
target_link_libraries(spiffe PRIVATE ${CURL_LIBRARIES} -lpthread -ldl)
target_include_directories(
    spiffe
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/SimpleProtos ${CURL_INCLUDE_DIRS}
)

# GoogleTest
find_package(GTest REQUIRED)

enable_testing()
include(GoogleTest)

# Unit Tests
add_executable(unit_tests 
    test/der_test.cpp 
    test/grpc_framing_test.cpp
)
target_link_libraries(unit_tests PRIVATE spiffe GTest::gtest_main)
target_include_directories(unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src) # Access internal headers

gtest_discover_tests(unit_tests)

# Fuzzing Target
if(ENABLE_FUZZING)
    add_executable(der_fuzz test/der_fuzz.cpp)
    target_link_libraries(der_fuzz PRIVATE spiffe)
    target_compile_options(der_fuzz PRIVATE -fsanitize=fuzzer)
    target_link_options(der_fuzz PRIVATE -fsanitize=fuzzer)
    target_include_directories(der_fuzz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
endif()

# Manual test
add_executable(manual_test test/main.cpp)
target_link_libraries(manual_test PRIVATE spiffe)
target_include_directories(manual_test PRIVATE spiffe)
