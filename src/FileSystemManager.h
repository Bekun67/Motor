#pragma once
#include <string>
#include <filesystem>

namespace FileSystemManager {
    // Create all necessary directories for the engine
    void InitializeDirectories();

    // Check if a file exists
    bool FileExists(const std::string& path);

    // Get the modification time of a file
    std::filesystem::file_time_type GetFileModificationTime(const std::string& path);

    // Check if FBX needs reimport (compare with custom file)
    bool NeedsReimport(const std::string& fbxPath, const std::string& customPath);

    // Scene file operations
    std::string GetSceneExtension();
    std::string GetScenesDirectory();
}