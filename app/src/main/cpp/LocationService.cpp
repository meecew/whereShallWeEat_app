#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <android/log.h>

#include "LocationService.h"

#define LOG_TAG "LocationService"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using json = nlohmann::json;

LocationService::LocationService() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

LocationService::~LocationService() {
    curl_global_cleanup();
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

Coordinates LocationService::getCurrentCoord() {
    CURL* curl = curl_easy_init();
    Coordinates coords;
    std::string responseBuffer;

    if (!curl) {
        LOGE("Failed to initialize CURL");
        return coords;
    }

    std::string url = "http://ip-api.com/json/";

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: myApp/1.0");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        LOGI("Location CURL success");
        try {
            json data = json::parse(responseBuffer);
            if (data["status"] == "success") {
                coords.lat = data["lat"];
                coords.lon = data["lon"];
                coords.success = true;
            } else {
                LOGE("IP-API error: %s", data["message"].get<std::string>().c_str());
            }
        } catch (const json::parse_error& e) {
            LOGE("JSON parse error: %s", e.what());
            std::cerr << e.what() << std::endl;
        }
    } else {
        LOGE("Location CURL failed: %s", curl_easy_strerror(res));
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return coords;

}

void LocationService::setGpsCoord(double lat, double lon) {
    lastGpsCoords.lat = lat;
    lastGpsCoords.lon = lon;
    lastGpsCoords.success = true;
}

Coordinates LocationService::getGpsCoord() {
    return lastGpsCoords;
}

void LocationService::printAddress(double lat, double lon) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return;
    }

    std::string url = "https://nominatim.openstreetmap.org/reverse?format=json&addressdetails=1&lat=" + std::to_string(lat) + "&lon=" + std::to_string(lon);
    std::string responseBuffer;
    
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: whereShallWeEat/1.0 (meowcowbusiness@gmail.com)");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
    if (curl_easy_perform(curl) == CURLE_OK) {
        try {
            json data = json::parse(responseBuffer);
            if (data.contains("address") && data["address"].is_object()) {
                for (auto& [key, value] : data["address"].items()) {
                    std::cout << key << ": " << value.get<std::string>() << std::endl;
                }
            }
        } catch (const json::parse_error& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}