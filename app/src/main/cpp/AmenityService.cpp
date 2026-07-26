#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <android/log.h>
#include "AmenityService.h"

#define LOG_TAG "AmenityService"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


using json = nlohmann::json;

#ifndef MATH_PI
#define MATH_PI 3.14159265358979323846264
#endif

static double degToRad(double deg){
    return deg * (MATH_PI / 180.0);
}

static double haversine(double lat1, double lon1, double lat2, double lon2) {
    constexpr double EARTH_RADIUS = 6371.0;

    lat1 = degToRad(lat1);
    lon1 = degToRad(lon1);
    lat2 = degToRad(lat2);
    lon2 = degToRad(lon2);

    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    double a = std::sin(dlat / 2) * std::sin(dlat / 2) + std::cos(lat1) * std::cos(lat2) * std::sin(dlon / 2) * std::sin(dlon / 2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return EARTH_RADIUS * c;
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append(static_cast<char*>(contents), total_size);
    return total_size;
}

std::vector<Amenity> AmenityService::fetchNearbyAmenities(double lat, double lon, double radius) {
    std::vector<Amenity> amenities;
    CURL* curl = curl_easy_init();
    if (!curl) return amenities;

    std::string url = "http://overpass-api.de/api/interpreter";
    std::string responseBuffer;

    std::string query = "[out:json][timeout:25];"
                        "nwr[\"amenity\"~\"^(restaurant|cafe|fast_food|food_court|pub|ice_cream|canteen|bistro)$\"](around:" + std::to_string(radius) + ","
                        + std::to_string(lat) + "," + std::to_string(lon) + ");"
                        "out center;";

    char* encodedQuery = curl_easy_escape(curl, query.c_str(), query.length());
    std::string postData = "data=" + std::string(encodedQuery);
    curl_free(encodedQuery);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: myApp/1.0");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        LOGI("CURL request successful");
        try {
            json data = json::parse(responseBuffer);
            
            if (data.contains("elements") && data["elements"].is_array()) {
                for (const auto& elem : data["elements"]) {
                    if (elem.contains("tags") && elem["tags"].contains("amenity")) {
                        Amenity item;
                        item.type = elem["tags"]["amenity"].get<std::string>();
                        

                        if (elem["tags"].contains("name")) {
                            item.name = elem["tags"]["name"].get<std::string>();
                        } else {
                            item.name = "Unnamed " + item.type;
                        }

                        if (elem.contains("lat") && elem.contains("lon")) {
                            item.lat = elem["lat"].get<double>();
                            item.lon = elem["lon"].get<double>();
                        } else if (elem.contains("center")) {
                            item.lat = elem["center"]["lat"].get<double>();
                            item.lon = elem["center"]["lon"].get<double>();
                        }
                        item.distance = haversine(lat, lon, item.lat, item.lon);
                        amenities.push_back(item);
                    }
                }
            }
        } catch (const json::parse_error& e) {
            LOGE("JSON parse error: %s", e.what());
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            std::cerr << responseBuffer << std::endl;
            std::cerr << "server is too busy";
        }
    } else {
        LOGE("CURL failed: %s", curl_easy_strerror(res));
    }
    std::sort(amenities.begin(), amenities.end(), [](const Amenity& a, const Amenity& b) {return a.distance < b.distance;});

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return amenities;
}