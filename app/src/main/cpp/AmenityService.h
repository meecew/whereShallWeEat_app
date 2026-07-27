#ifndef AMENITY_SERVICE_H
#define AMENITY_SERVICE_H

#include <string>
#include <vector>
#include <unordered_set>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

struct Amenity {
    std::string name;
    std::string type; 
    double lat = 0.0;
    double lon = 0.0;
    double distance = 0.0;
};

class AmenityService {
public:
    AmenityService() = default;
    ~AmenityService() = default;

    std::unordered_set<std::string> includedTypes = {
        "restaurant"
    };
    
    std::vector<Amenity> fetchNearbyAmenities(double lat, double lon, double radius, std::string& outError, AAssetManager* assetManager);

private:
    std::vector<Amenity> fetchFromLocalJson(AAssetManager* assetManager, double userLat, double userLon, double radius);
    bool isInsideBC(double lat, double lon);
};

#endif