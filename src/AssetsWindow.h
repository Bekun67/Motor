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
    MODEL_SOURCE,     
    TEXTURE_SOURCE,   
    UNKNOWN
};

struct AssetInfo
{
    std::string name;
    std::string path;
    std::string relativePath;
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

    void CheckForChanges();
    bool ImportDroppedFile(const std::string& filePath);
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

    void BeginDragDropSource(const AssetInfo& asset);
    void HandleSceneFileDrop(const std::string& scenePath);
    void HandleMeshFileDrop(const std::string& meshPath, float mouseX, float mouseY);
    void HandleTextureFileDrop(const std::string& texturePath);

    AssetType GetAssetTypeFromExtension(const std::string& extension);
    ImVec4 GetAssetTypeColor(AssetType type);
    std::string FormatFileSize(uint64_t bytes);
    const char* GetAssetTypeIcon(AssetType type);

    void HandleAssetDragToScene(const AssetInfo& asset);
    void HandleAssetDoubleClick(const AssetInfo& asset);
    bool IsModelFile(const std::string& extension);
    bool IsTextureFile(const std::string& extension);
    bool IsSceneFile(const std::string& extension);

private:
    std::string assetsRoot;
    std::string currentPath;
    std::vector<AssetInfo> currentAssets;
    std::vector<std::string> allFolders;

    bool gridView;
    float thumbnailSize;
    float padding;

    float timeSinceLastCheck;
    float checkInterval;
    std::map<std::string, fs::file_time_type> fileTimestamps;

    char searchBuffer[256];

    int selectedIndex;
};