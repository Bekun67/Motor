#pragma once
#include "EditorWindow.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <map>
#include <filesystem>

namespace fs = std::filesystem;

enum class AssetType
{
    FOLDER,
    MESH,
    TEXTURE,
    SCENE,
    UNKNOWN
};

struct AssetInfo
{
    std::string name;
    std::string path;
    std::string relativePath; // Relative to current folder
    AssetType type;
    bool isDirectory;
    int referenceCount;
    bool isInMemory;
    uint64_t fileSize;

    AssetInfo() : type(AssetType::UNKNOWN), isDirectory(false),
        referenceCount(0), isInMemory(false), fileSize(0) {
    }
};

class AssetsWindow : public EditorWindow
{
public:
    AssetsWindow(ModuleEditor* editor);
    ~AssetsWindow() override = default;

    void Draw() override;

    // Monitor assets folder every second
    void CheckForChanges();

    // Import dropped file from external source
    bool ImportDroppedFile(const std::string& filePath);

    // Get reference count for a resource
    int GetReferenceCount(const std::string& libraryPath);

private:
    void DrawToolbar();
    void DrawNavigationBar();
    void DrawFolderTree();
    void DrawAssetGrid();
    void DrawAssetList();
    void DrawFileDropArea();
    void DrawAssetContextMenu(const AssetInfo& asset);

    void RefreshCurrentFolder();
    void NavigateToFolder(const std::string& folderPath);
    void NavigateUp();

    void ScanFolderRecursive(const std::string& path, std::vector<std::string>& outFolders);

    // Drag and drop
    void BeginDragDropSource(const AssetInfo& asset);

    // Helpers
    AssetType GetAssetTypeFromExtension(const std::string& extension);
    ImVec4 GetAssetTypeColor(AssetType type);
    std::string FormatFileSize(uint64_t bytes);
    const char* GetAssetTypeIcon(AssetType type);

private:
    std::string libraryRoot;
    std::string currentPath;
    std::vector<AssetInfo> currentAssets;
    std::vector<std::string> allFolders; // For folder tree

    // View settings
    bool gridView;
    float thumbnailSize;
    float padding;

    // Monitoring
    float timeSinceLastCheck;
    float checkInterval;
    std::map<std::string, fs::file_time_type> fileTimestamps;

    // Search
    char searchBuffer[256];

    // Selection
    int selectedIndex;
};