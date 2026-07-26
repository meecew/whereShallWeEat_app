#include <jni.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include "AmenityService.h"
#include "LocationService.h"

// Static instance to persist state (like GPS coordinates) across JNI calls
static LocationService gLocServ;

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_MainActivity_fetchAmenitiesPlainTextNative(
        JNIEnv* env,
        jobject /* this */,
        jdouble lat,
        jdouble lon,
        jdouble radius) {

    AmenityService amenityServ;
    auto amenities = amenityServ.fetchNearbyAmenities(lat, lon, radius);

    std::map<std::string, std::vector<Amenity>> amenitiesGroup;
    for (const auto& item : amenities) {
        amenitiesGroup[item.type].push_back(item);
    }

    std::ostringstream ss;
    ss << "my location: " << lat << ", " << lon << "\n";
    ss << "\ntotal foodie places: " << amenities.size() << "\n\n";

    for (const auto& [type, items] : amenitiesGroup) {
        ss << type << ": " << items.size() << "\n";
        for (const auto& item : items) {
            ss << "  " << item.name << " - ";
            if (item.distance < 1.0) {
                ss << std::fixed << std::setprecision(2) << item.distance * 1000.0 << " m\n";
            } else {
                ss << std::fixed << std::setprecision(2) << item.distance << " km\n";
            }
        }
        ss << "\n";
    }

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" JNIEXPORT jdoubleArray JNICALL
Java_com_example_myapplication_MainActivity_getIpLocationNative(
        JNIEnv* env,
        jobject /* this */) {

    Coordinates coords = gLocServ.getCurrentCoord();

    jdoubleArray result = env->NewDoubleArray(3);
    jdouble fill[3] = { coords.lat, coords.lon, coords.success ? 1.0 : 0.0 };
    env->SetDoubleArrayRegion(result, 0, 3, fill);
    return result;
}

extern "C" JNIEXPORT jdoubleArray JNICALL
Java_com_example_myapplication_MainActivity_getGpsLocationNative(
        JNIEnv* env,
        jobject /* this */) {

    Coordinates coords = gLocServ.getGpsCoord();

    jdoubleArray result = env->NewDoubleArray(3);
    jdouble fill[3] = { coords.lat, coords.lon, coords.success ? 1.0 : 0.0 };
    env->SetDoubleArrayRegion(result, 0, 3, fill);
    return result;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_myapplication_MainActivity_updateGpsLocationNative(
        JNIEnv* env,
        jobject /* this */,
        jdouble lat,
        jdouble lon) {

    gLocServ.setGpsCoord(lat, lon);
}
