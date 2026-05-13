#include <iostream>
#include "GPSCSVReader.h"
#include "DataCleaner.h"

int main() {
    GPSCSVReader reader("../data/gps_log.csv");
    std::vector<GPSData> datos = reader.readAll();

    DataCleaner cleaner;
    cleaner.clean(datos);

    return 0;
}
