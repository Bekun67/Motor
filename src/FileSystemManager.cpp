#include "FileSystemManager.h"
#include <iostream>

namespace fs = std::filesystem;

namespace FileSystemManager {

    void InitializeDirectories() {
        std::vector<std::string> directories = {
            "Assets",
            "Assets/Models",
            "Assets/Textures",
            "Library",
            "Library/Meshes",
            "Library/Materials",
            "Library/Models"
        };

        for (const auto& dir : directories) {
            if (!fs::exists(dir)) {
                fs::create_directories(dir);
                std::cout << "[FileSystem] Created directory: " << dir << std::endl;
            }
        }
    }

    bool FileExists(const std::string& path) {
        return fs::exists(path);
    }

    std::filesystem::file_time_type GetFileModificationTime(const std::string& path) {
        if (!fs::exists(path)) {
            return std::filesystem::file_time_type::min();
        }
        return fs::last_write_time(path);
    }

    bool NeedsReimport(const std::string& fbxPath, const std::string& customPath) {
        if (!FileExists(customPath)) {
            return true; // Custom file doesn't exist, needs import
        }

        auto fbxTime = GetFileModificationTime(fbxPath);
        auto customTime = GetFileModificationTime(customPath);

        return fbxTime > customTime; // FBX is newer than custom file
    }
}