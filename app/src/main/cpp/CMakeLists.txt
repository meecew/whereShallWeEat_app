cmake_minimum_required(VERSION 3.18)
project("myApp" CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 1. Fetch nlohmann/json
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# 2. Fetch libcurl
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CURL_ENABLE_SSL OFF CACHE BOOL "" FORCE) # Turn OFF if testing without native OpenSSL build on Android NDK

FetchContent_Declare(
        curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG        curl-8_5_0
)
FetchContent_MakeAvailable(curl)

# 3. Create SHARED library for JNI
add_library(
        native-lib
        SHARED
        native-lib.cpp
        LocationService.cpp
        AmenityService.cpp
)

find_library(log-lib log)

target_link_libraries(
        native-lib
        PRIVATE
        nlohmann_json::nlohmann_json
        libcurl
        ${log-lib}
)