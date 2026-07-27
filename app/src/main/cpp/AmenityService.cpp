#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <android/log.h>
#include <algorithm>
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

std::vector<Amenity> AmenityService::fetchNearbyAmenities(double lat, double lon, double radius, std::string& outError, AAssetManager* assetManager) {
    // 1. Check if we are in British Columbia for offline mode
    if (isInsideBC(lat, lon)) {
        return fetchFromLocalJson(assetManager, lat, lon, radius);
    }


    std::vector<Amenity> amenities;
    outError = "";
    CURL* curl = curl_easy_init();
    if (!curl) {
        return amenities;
    }

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
    headers = curl_slist_append(headers, "User-Agent: whereShallWeEat/1.0");

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

            if (amenities.empty() && responseBuffer.find("remark") != std::string::npos) {
                 // Check for specific Overpass errors like "Rate limit reached" or "Server busy"
                 if (responseBuffer.find("busy") != std::string::npos) {
                     outError = "Server is too busy. Please try again in 30 seconds.";
                 } else if (responseBuffer.find("rate limit") != std::string::npos) {
                     outError = "Rate limit reached. Slow down!";
                 }
            }

        } catch (const json::parse_error& e) {
            LOGE("JSON parse error: %s", e.what());
            if (responseBuffer.find("Rate limit") != std::string::npos ||
                responseBuffer.find("busy") != std::string::npos ||
                responseBuffer.find("Too Many Requests") != std::string::npos) {
                outError = "Server is busy or Rate Limit reached.";
            } else {
                outError = "Failed to parse server response.";
            }
        }
    } else {
        outError = "Network error: " + std::string(curl_easy_strerror(res));
        LOGE("CURL failed: %s", curl_easy_strerror(res));
    }
    std::sort(amenities.begin(), amenities.end(), [](const Amenity& a, const Amenity& b) {return a.distance < b.distance;});

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return amenities;
}

bool AmenityService::isInsideBC(double lat, double lon) {
    // Rough bounding box for British Columbia
    return (lat >= 48.3 && lat <= 60.0 && lon >= -139.0 && lon <= -114.0);
}

std::vector<Amenity> AmenityService::fetchFromLocalJson(AAssetManager* assetManager, double userLat, double userLon, double radius) {
    std::vector<Amenity> results;
    if (!assetManager) {
        LOGE("AssetManager is null!");
        return results;
    }

    // 1. Open the local file from assets
    AAsset* asset = AAssetManager_open(assetManager, "bc_foodies_database.json", AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("Could not open local BC database asset!");
        return results;
    }

    // 2. Read the entire file into memory
    off_t size = AAsset_getLength(asset);
    std::string buffer(size, '\0');
    AAsset_read(asset, &buffer[0], size);
    AAsset_close(asset);

    try {
        // 3. Parse the JSON
        json data = json::parse(buffer);
        double radiusInKm = radius / 1000.0;

        LOGI("Parsing local JSON with %zu entries...", data.size());

        for (const auto& item : data) {
            try {
                double itemLat = item["lat"].get<double>();
                double itemLon = item["lon"].get<double>();

                // 4. Calculate distance
                double dist = haversine(userLat, userLon, itemLat, itemLon);

                // 5. Only include if within the requested radius
                if (dist <= radiusInKm) {
                    Amenity am;
                    am.name = item.value("name", "Unnamed");
                    am.type = item.value("type", "food");
                    am.lat = itemLat;
                    am.lon = itemLon;
                    am.distance = dist;
                    results.push_back(am);
                }
            } catch (const std::exception& inner) {
                // Skip malformed entries
            }
        }
    } catch (const json::parse_error& e) {
        LOGE("JSON Parse Error: %s", e.what());
    } catch (const std::exception& e) {
        LOGE("Error parsing local JSON: %s", e.what());
    }

    // Sort by distance
    std::sort(results.begin(), results.end(), [](const Amenity& a, const Amenity& b) {
        return a.distance < b.distance;
    });

    return results;
}
