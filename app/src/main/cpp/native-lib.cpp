#include <jni.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <random>
#include "AmenityService.h"
#include "LocationService.h"

// Static instance to persist state (like GPS coordinates) across JNI calls
static LocationService gLocServ;

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_MainActivity_fetchAmenitiesPlainTextNative(
        JNIEnv* env,
        jobject thiz,
        jdouble lat,
        jdouble lon,
        jdouble radius) {

    // Get the AssetManager from the Java side
    jclass mainActivityClass = env->GetObjectClass(thiz);
    jmethodID getAssetsMethod = env->GetMethodID(mainActivityClass, "getAssets", "()Landroid/content/res/AssetManager;");
    jobject assetManagerObj = env->CallObjectMethod(thiz, getAssetsMethod);
    AAssetManager* assetManager = AAssetManager_fromJava(env, assetManagerObj);

    AmenityService amenityServ;
    std::string errorMsg;
    auto amenities = amenityServ.fetchNearbyAmenities(lat, lon, radius, errorMsg, assetManager);

    std::ostringstream ss;
    if (!errorMsg.empty()) {
        ss << "server busy :(" << "\n\n";
    } else {
        ss << "total foodie places: " << amenities.size() << "\n\n";
    }

    std::map<std::string, std::vector<Amenity>> amenitiesGroup;
    for (const auto& item : amenities) {
        amenitiesGroup[item.type].push_back(item);
    }


    for (const auto& [type, items] : amenitiesGroup) {
        ss << "<b>" << type << ": " << items.size() << "</b><br>";
        for (const auto& item : items) {
            if (item.distance < 1.0) {
                ss << std::fixed << std::setprecision(2) << item.distance * 1000.0 << " m";
            } else {
                ss << std::fixed << std::setprecision(2) << item.distance << " km";
            }
            ss << " - <a href=\"geo:" << std::fixed << std::setprecision(6) << item.lat << "," << item.lon << "?q=" << item.lat << "," << item.lon << "(" << item.name << ")\">" << item.name << "</a>\n";
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

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_MainActivity_fetchRandomAmenityNative(
        JNIEnv* env,
        jobject thiz,
        jdouble lat,
        jdouble lon,
        jdouble radius) {

    // Get AssetManager
    jclass mainActivityClass = env->GetObjectClass(thiz);
    jmethodID getAssetsMethod = env->GetMethodID(mainActivityClass, "getAssets", "()Landroid/content/res/AssetManager;");
    jobject assetManagerObj = env->CallObjectMethod(thiz, getAssetsMethod);
    AAssetManager* assetManager = AAssetManager_fromJava(env, assetManagerObj);

    AmenityService amenityServ;
    std::string errorMsg;
    auto amenities = amenityServ.fetchNearbyAmenities(lat, lon, radius, errorMsg, assetManager);

    std::ostringstream ss;
    if (!errorMsg.empty()) {
        ss << "server busy :(" << "\n\n";
    } else if (amenities.empty()) {
        ss << "theres nothing here bro " << radius << "m\n";
    } else {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, amenities.size() - 1);
        int randomIndex = dis(gen);
        Amenity selected = amenities[randomIndex];

        ss << "you will eat: " << selected.name << "!!" << std::endl << std::endl;


        std::map<std::string, std::vector<Amenity>> amenitiesGroup;
        for (const auto& item : amenities) {
            amenitiesGroup[item.type].push_back(item);
        }

        for (const auto& [type, items] : amenitiesGroup) {
            ss << type << ": " << items.size() << "\n";
            for (const auto& item : items) {
                bool isPicked = (item.name == selected.name && item.lat == selected.lat && item.lon == selected.lon);


                if (item.distance < 1.0) {
                    ss << std::fixed << std::setprecision(2) << item.distance * 1000.0 << " m";
                } else {
                    ss << std::fixed << std::setprecision(2) << item.distance << " km";
                }
                ss << " - <a href=\"geo:" << std::fixed << std::setprecision(6) << item.lat << "," << item.lon << "?q=" << item.lat << "," << item.lon << "(" << item.name << ")\">" << item.name << "</a>";

                if (isPicked) ss << std::endl << "<font color='#FF5722'><b>^^^^THIS GUY RIGHT HERE!!</b></font>";
                ss << "\n";
            }
            ss << "\n";
        }
    }

    return env->NewStringUTF(ss.str().c_str());
}
