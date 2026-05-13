#include "FileHelper.h"
#include <fstream>
#include <filesystem>

bool FileHelper::exists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

void FileHelper::ensureExists(const std::string& path) {
    if (!exists(path)) {
        std::ofstream create(path); 
        create.close();
    }
}

void FileHelper::writeHeaderIfNew(const std::string& path, const std::string& header) {
    std::ifstream check(path);
    if (check.peek() == std::ifstream::traits_type::eof()) {
        std::ofstream file(path);
        file << header << "\n";
        file.close();
    }
}
