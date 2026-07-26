#ifndef LOCATION_SERVICE_H
#define LOCATION_SERVICE_H

#include <string>

struct Coordinates {
    double lat = 0.0;
    double lon = 0.0;
    bool success = false;
};

class LocationService {
public:
    LocationService();
    ~LocationService();

    Coordinates getCurrentCoord();
    void setGpsCoord(double lat, double lon);
    Coordinates getGpsCoord();
    void printAddress(double lat, double lon);
private:
    Coordinates lastGpsCoords;
};

#endif