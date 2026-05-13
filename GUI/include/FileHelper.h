#ifndef FILEHELPER_H
#define FILEHELPER_H

#include <string>

class FileHelper {
public:
    static bool exists(const std::string& path);
    static void ensureExists(const std::string& path);
    static void writeHeaderIfNew(const std::string& path, const std::string& header);
};

#endif
