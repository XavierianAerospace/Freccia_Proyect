#ifndef GPSDATA_H
#define GPSDATA_H

#include <string>

struct GPSData {
    double latitude;
    double longitude;
    std::string date;
    std::string utc_time;
    int satellites;
    float hdop;
};

#endif
