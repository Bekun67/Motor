#include "AssetsWindow.h"
#include "ModuleEditor.h"
#include "Application.h"
#include "Window.h"
#include "OpenGL.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "ResourceManager.h"
#include "LoadFBX.h"
#include <algorithm>
#include <functional>

AssetsWindow::AssetsWindow(ModuleEditor* editor)
    : EditorWindow(editor, "Assets")
    , libraryRoot("Library")
    , currentPath("Library")
    , gridView(true)
    , thumbnailSize(80.0f)
    , padding(16.0f)
    , timeSinceLastCheck(0.0f)
    , checkInterval(1.0f)
    , selectedIndex(-1)
{
    memset(searchBuffer, 0, sizeof(searchBuffer));

    // Only refresh if Library exists
    if (fs::exists(libraryRoot) && fs::is_directory(libraryRoot))
    {
        try
        {
            RefreshCurrentFolder();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Error initializing AssetsWindow: " + std::string(e.what()));
        }
    }
    else
    {
        LOG("Library folder does not exist yet - will be created when importing assets");
    }
}
void AssetsWindow::Draw()
{
    if (!visible) return;

    Window* window = Application::GetInstance().window.get();
    int windowWidth, windowHeight;
    window->GetWindowSize(windowWidth, windowHeight);

    // Position where console was
    if (editor->firstTimeSetup || editor->useAdaptiveLayout)
    {
        float x = windowWidth * editor->layout.consoleXPercent;
        float y = windowHeight * editor->layout.consoleYPercent;
        float width = windowWidth * editor->layout.consoleWidthPercent;
        float height = windowHeight * editor->layout.consoleHeightPercent - editor->layout.marginY;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }

    ImGui::Begin("Assets", &visible);

    // Monitor for changes every second
    timeSinceLastCheck += ImGui::GetIO().DeltaTime;
    if (timeSinceLastCheck >= checkInterval)
    {
        CheckForChanges();
        timeSinceLastCheck = 0.0f;
    }

    DrawToolbar();
    ImGui::Separator();

    // Two-panel layout: Folder tree | Asset view
    float treeWidth = 200.0f;

    ImGui::BeginChild("FolderTree", ImVec2(treeWidth, -25), true);
    DrawFolderTree();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("AssetView", ImVec2(0, -25), true);

    DrawNavigationBar();
    ImGui::Separator();

    DrawFileDropArea();

    if (gridView)
    {
        DrawAssetGrid();
    }
    else
    {
        DrawAssetList();
    }

    ImGui::EndChild();

    // Status bar
    ImGui::Separator();
    int inMemoryCount = std::count_if(currentAssets.begin(), currentAssets.end(),
        [](const AssetInfo& a) { return a.isInMemory; });
    ImGui::Text("Assets: %d | In Memory: %d", (int)currentAssets.size(), inMemoryCount);

    ImGui::End();
}

void AssetsWindow::DrawToolbar()
{
    // Navigation buttons
    if (ImGui::Button("< Back"))
    {
        NavigateUp();
    }

    ImGui::SameLine();

    if (ImGui::Button("Refresh"))
    {
        RefreshCurrentFolder();
    }

    ImGui::SameLine();

    // View mode toggle
    if (ImGui::Button(gridView ? "List" : "Grid"))
    {
        gridView = !gridView;
    }

    ImGui::SameLine();

    // Search box
    ImGui::PushItemWidth(200.0f);
    if (ImGui::InputTextWithHint("##search", "Search...", searchBuffer, sizeof(searchBuffer)))
    {
        RefreshCurrentFolder();
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();

    // Grid size slider (only in grid view)
    if (gridView)
    {
        ImGui::PushItemWidth(100.0f);
        ImGui::SliderFloat("##size", &thumbnailSize, 50.0f, 150.0f, "Size");
        ImGui::PopItemWidth();
    }
}

void AssetsWindow::DrawNavigationBar()
{
    // Show current path as breadcrumb
    std::string displayPath = currentPath;

    // Replace backslashes with forward slashes
    std::replace(displayPath.begin(), displayPath.end(), '\\', '/');

    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Current: %s", displayPath.c_str());
}

void AssetsWindow::DrawFolderTree()
{
    // Early exit if Library doesn't exist yet
    if (!fs::exists(libraryRoot))
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Library folder not created yet");
        ImGui::TextWrapped("Import a model or texture to create it");
        return;
    }

    // Lambda to draw tree nodes recursively
    std::function<void(const std::string&, int)> drawNode = [&](const std::string& path, int depth)
        {
            // Safety checks
            if (path.empty())
                return;

            if (!fs::exists(path))
                return;

            if (!fs::is_directory(path))
                return;

            std::string folderName = fs::path(path).filename().string();
            if (folderName.empty())
                folderName = "Library";

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

            // Highlight if selected
            if (path == currentPath)
                flags |= ImGuiTreeNodeFlags_Selected;

            // Check if has subfolders
            bool hasSubfolders = false;
            try
            {
                for (const auto& entry : fs::directory_iterator(path))
                {
                    if (entry.is_directory())
                    {
                        hasSubfolders = true;
                        break;
                    }
                }
            }
            catch (const std::exception& e)
            {
                // Can't access folder, skip it
                return;
            }

            if (!hasSubfolders)
                flags |= ImGuiTreeNodeFlags_Leaf;

            // Create unique ID using hash of path
            std::hash<std::string> hasher;
            size_t pathHash = hasher(path);

            ImGui::PushID((int)pathHash);

            // Icon + name format
            std::string label = std::string(GetAssetTypeIcon(AssetType::FOLDER)) + " " + folderName;

            bool nodeOpen = ImGui::TreeNodeEx("##treenode", flags, "%s", label.c_str());

            // Click to navigate
            if (ImGui::IsItemClicked())
            {
                NavigateToFolder(path);
            }

            if (nodeOpen)
            {
                try
                {
                    std::vector<std::string> subfolders;

                    // Collect subfolders first
                    for (const auto& entry : fs::directory_iterator(path))
                    {
                        if (entry.is_directory())
                        {
                            subfolders.push_back(entry.path().string());
                        }
                    }

                    // Sort subfolders
                    std::sort(subfolders.begin(), subfolders.end());

                    // Draw each subfolder
                    for (const auto& subfolder : subfolders)
                    {
                        drawNode(subfolder, depth + 1);
                    }
                }
                catch (const std::exception& e)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error reading folder");
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        };

    // Start from Library root with safety check
    try
    {
        drawNode(libraryRoot, 0);
    }
    catch (const std::exception& e)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error displaying folder tree");
        LOG_ERROR("Folder tree error: " + std::string(e.what()));
    }
}

void AssetsWindow::DrawAssetGrid()
{
    if (currentAssets.empty())
    {
        ImVec2 region = ImGui::GetContentRegionAvail();
        ImVec2 textSize = ImGui::CalcTextSize("No assets in this folder");
        ImVec2 textPos = ImVec2(region.x * 0.5f - textSize.x * 0.5f, region.y * 0.5f);

        ImGui::SetCursorPos(textPos);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No assets in this folder");
        return;
    }

    float cellSize = thumbnailSize + padding;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = std::max(1, (int)(panelWidth / cellSize));

    int currentColumn = 0;

    for (int i = 0; i < (int)currentAssets.size(); ++i)
    {
        const AssetInfo& asset = currentAssets[i];

        // Create unique ID
        ImGui::PushID(i + 1000); // Offset to avoid ID conflicts
        ImGui::BeginGroup();

        // Asset icon button
        ImVec4 iconColor = GetAssetTypeColor(asset.type);

        // Highlight if selected
        if (i == selectedIndex)
        {
            iconColor = ImVec4(
                std::min(iconColor.x * 1.3f, 1.0f),
                std::min(iconColor.y * 1.3f, 1.0f),
                std::min(iconColor.z * 1.3f, 1.0f),
                1.0f
            );
        }

        ImGui::PushStyleColor(ImGuiCol_Button, iconColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            ImVec4(
                std::min(iconColor.x * 1.2f, 1.0f),
                std::min(iconColor.y * 1.2f, 1.0f),
                std::min(iconColor.z * 1.2f, 1.0f),
                1.0f
            )
        );
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
            ImVec4(iconColor.x * 0.8f, iconColor.y * 0.8f, iconColor.z * 0.8f, 1.0f)
        );

        // Create unique label
        std::string buttonLabel = std::string(GetAssetTypeIcon(asset.type)) + "##btn";
        bool clicked = ImGui::Button(buttonLabel.c_str(), ImVec2(thumbnailSize, thumbnailSize));

        ImGui::PopStyleColor(3);

        if (clicked)
        {
            selectedIndex = i;
        }

        // Double-click to navigate folder
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            if (asset.isDirectory)
            {
                NavigateToFolder(asset.path);
            }
        }

        // Drag source for assets
        if (!asset.isDirectory)
        {
            BeginDragDropSource(asset);
        }

        // Asset name (wrapped)
        float textWidth = thumbnailSize;
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + textWidth);

        // Truncate very long names
        std::string displayName = asset.name;
        if (displayName.length() > 20)
        {
            displayName = displayName.substr(0, 17) + "...";
        }

        ImGui::TextWrapped("%s", displayName.c_str());
        ImGui::PopTextWrapPos();

        // Show reference count if in memory
        if (asset.isInMemory && asset.referenceCount > 0)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Refs: %d", asset.referenceCount);
        }

        ImGui::EndGroup();

        // Context menu
        std::string popupID = "ctx##" + std::to_string(i);
        if (ImGui::BeginPopupContextItem(popupID.c_str()))
        {
            DrawAssetContextMenu(asset);
            ImGui::EndPopup();
        }

        // Tooltip with info
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Name: %s", asset.name.c_str());
            if (!asset.isDirectory)
            {
                ImGui::Text("Size: %s", FormatFileSize(asset.fileSize).c_str());
                const char* typeStr = "Unknown";
                switch (asset.type)
                {
                case AssetType::MESH: typeStr = "Mesh"; break;
                case AssetType::TEXTURE: typeStr = "Texture"; break;
                case AssetType::SCENE: typeStr = "Scene"; break;
                default: break;
                }
                ImGui::Text("Type: %s", typeStr);
            }
            ImGui::EndTooltip();
        }

        ImGui::PopID();

        currentColumn++;
        if (currentColumn < columnCount)
        {
            ImGui::SameLine();
        }
        else
        {
            currentColumn = 0;
        }
    }
}

void AssetsWindow::DrawAssetList()
{
    if (currentAssets.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No assets in this folder");
        return;
    }

    if (ImGui::BeginTable("AssetListTable", 4,
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Refs", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < (int)currentAssets.size(); ++i)
        {
            const AssetInfo& asset = currentAssets[i];

            ImGui::TableNextRow();

            // Use unique ID
            ImGui::PushID(i + 2000); // Different offset for list view

            // Name column
            ImGui::TableNextColumn();

            ImVec4 color = GetAssetTypeColor(asset.type);

            // Selectable row
            bool isSelected = (i == selectedIndex);
            std::string selectableID = "##sel" + std::to_string(i);

            if (ImGui::Selectable(selectableID.c_str(), isSelected,
                ImGuiSelectableFlags_SpanAllColumns |
                ImGuiSelectableFlags_AllowItemOverlap))
            {
                selectedIndex = i;
            }

            // Double-click to open folder
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                if (asset.isDirectory)
                {
                    NavigateToFolder(asset.path);
                }
            }

            ImGui::SameLine();
            ImGui::TextColored(color, "%s %s", GetAssetTypeIcon(asset.type), asset.name.c_str());

            // Drag source
            if (!asset.isDirectory)
            {
                BeginDragDropSource(asset);
            }

            // Type column
            ImGui::TableNextColumn();
            if (asset.isDirectory)
            {
                ImGui::Text("Folder");
            }
            else
            {
                const char* typeStr = "Unknown";
                switch (asset.type)
                {
                case AssetType::MESH: typeStr = "Mesh"; break;
                case AssetType::TEXTURE: typeStr = "Texture"; break;
                case AssetType::SCENE: typeStr = "Scene"; break;
                default: break;
                }
                ImGui::Text("%s", typeStr);
            }

            // Size column
            ImGui::TableNextColumn();
            if (!asset.isDirectory)
            {
                ImGui::Text("%s", FormatFileSize(asset.fileSize).c_str());
            }
            else
            {
                ImGui::Text("-");
            }

            // References column
            ImGui::TableNextColumn();
            if (asset.isInMemory && asset.referenceCount > 0)
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%d", asset.referenceCount);
            }
            else
            {
                ImGui::Text("-");
            }

            // Context menu
            std::string ctxID = "listctx##" + std::to_string(i);
            if (ImGui::BeginPopupContextItem(ctxID.c_str()))
            {
                DrawAssetContextMenu(asset);
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

void AssetsWindow::DrawFileDropArea()
{
    // Invisible button to capture drop area
    ImVec2 dropSize = ImGui::GetContentRegionAvail();
    dropSize.y = std::min(dropSize.y, 50.0f);

    ImGui::InvisibleButton("##dropzone", dropSize);

    // Visual feedback for drop zone
    if (ImGui::IsItemHovered())
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 pMin = ImGui::GetItemRectMin();
        ImVec2 pMax = ImGui::GetItemRectMax();
        drawList->AddRect(pMin, pMax, IM_COL32(100, 150, 255, 255), 4.0f, 0, 2.0f);
    }
}

void AssetsWindow::DrawAssetContextMenu(const AssetInfo& asset)
{
    if (ImGui::MenuItem("Show in Explorer"))
    {
        std::string command = "explorer /select,\"" + asset.path + "\"";
        system(command.c_str());
    }

    if (!asset.isDirectory)
    {
        ImGui::Separator();

        if (ImGui::MenuItem("Delete"))
        {
            try
            {
                fs::remove(asset.path);
                LOG("Deleted: " + asset.path);
                RefreshCurrentFolder();
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Failed to delete: " + std::string(e.what()));
            }
        }
    }
}

void AssetsWindow::RefreshCurrentFolder()
{
    currentAssets.clear();
    selectedIndex = -1;

    // Safety check
    if (currentPath.empty())
    {
        currentPath = libraryRoot;
    }

    if (!fs::exists(currentPath))
    {
        LOG_WARNING("Current path does not exist: " + currentPath);
        return;
    }

    if (!fs::is_directory(currentPath))
    {
        LOG_WARNING("Current path is not a directory: " + currentPath);
        currentPath = libraryRoot;
        return;
    }

    try
    {
        for (const auto& entry : fs::directory_iterator(currentPath))
        {
            try
            {
                AssetInfo info;
                info.path = entry.path().string();
                info.name = entry.path().filename().string();

                // Skip empty names
                if (info.name.empty())
                    continue;

                info.isDirectory = entry.is_directory();

                // Apply search filter
                if (strlen(searchBuffer) > 0)
                {
                    std::string nameLower = info.name;
                    std::string searchLower = searchBuffer;
                    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

                    if (nameLower.find(searchLower) == std::string::npos)
                        continue;
                }

                if (!info.isDirectory)
                {
                    try
                    {
                        info.fileSize = fs::file_size(entry.path());
                    }
                    catch (...)
                    {
                        info.fileSize = 0;
                    }

                    std::string ext = entry.path().extension().string();
                    info.type = GetAssetTypeFromExtension(ext);

                    // Check if in memory
                    info.referenceCount = GetReferenceCount(info.path);
                    info.isInMemory = (info.referenceCount > 0);

                    // Store timestamp for monitoring
                    try
                    {
                        fileTimestamps[info.path] = fs::last_write_time(entry.path());
                    }
                    catch (...) {}
                }
                else
                {
                    info.type = AssetType::FOLDER;
                }

                currentAssets.push_back(info);
            }
            catch (const std::exception& e)
            {
                // Skip problematic files
                continue;
            }
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error scanning folder '" + currentPath + "': " + std::string(e.what()));
        return;
    }

    // Sort: folders first, then by name
    std::sort(currentAssets.begin(), currentAssets.end(),
        [](const AssetInfo& a, const AssetInfo& b) {
            if (a.isDirectory != b.isDirectory)
                return a.isDirectory;
            return a.name < b.name;
        });
}

void AssetsWindow::CheckForChanges()
{
    if (!fs::exists(currentPath))
        return;

    bool needsRefresh = false;

    try
    {
        // Check for new or modified files
        for (const auto& entry : fs::directory_iterator(currentPath))
        {
            if (entry.is_directory())
                continue;

            std::string path = entry.path().string();
            auto newTime = fs::last_write_time(entry.path());

            auto it = fileTimestamps.find(path);
            if (it == fileTimestamps.end())
            {
                // New file detected
                LOG("New asset detected: " + path);
                needsRefresh = true;
            }
            else if (it->second != newTime)
            {
                // File modified
                LOG("Asset modified: " + path);
                fileTimestamps[path] = newTime;
                needsRefresh = true;
            }
        }

        // Check for deleted files
        std::vector<std::string> toRemove;
        for (auto& pair : fileTimestamps)
        {
            if (!fs::exists(pair.first))
            {
                LOG("Asset removed: " + pair.first);
                toRemove.push_back(pair.first);
                needsRefresh = true;
            }
        }

        for (const auto& path : toRemove)
        {
            fileTimestamps.erase(path);
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error checking for changes: " + std::string(e.what()));
    }

    if (needsRefresh)
    {
        RefreshCurrentFolder();
    }
}

void AssetsWindow::NavigateToFolder(const std::string& folderPath)
{
    if (fs::exists(folderPath) && fs::is_directory(folderPath))
    {
        currentPath = folderPath;
        RefreshCurrentFolder();
    }
}

void AssetsWindow::NavigateUp()
{
    fs::path currentPathObj(currentPath);

    if (currentPathObj.has_parent_path())
    {
        fs::path parent = currentPathObj.parent_path();

        // Don't go above Library root
        if (parent.string() == libraryRoot ||
            parent.string().find(libraryRoot) != std::string::npos)
        {
            NavigateToFolder(parent.string());
        }
    }
}

void AssetsWindow::ScanFolderRecursive(const std::string& path, std::vector<std::string>& outFolders)
{
    if (!fs::exists(path) || !fs::is_directory(path))
        return;

    try
    {
        for (const auto& entry : fs::directory_iterator(path))
        {
            if (entry.is_directory())
            {
                outFolders.push_back(entry.path().string());
                ScanFolderRecursive(entry.path().string(), outFolders);
            }
        }
    }
    catch (...) {}
}

bool AssetsWindow::ImportDroppedFile(const std::string& filePath)
{
    LOG("Importing file: " + filePath);

    std::string extension = fs::path(filePath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // Copy file to Assets folder if from external source
    // For now just log it
    LOG("File import requested: " + filePath);

    return true;
}

int AssetsWindow::GetReferenceCount(const std::string& libraryPath)
{
    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (!opengl)
        return 0;

    int count = 0;

    for (GameObject* go : opengl->gameObjects)
    {
        if (!go)
            continue;

        // Check mesh
        if (go->mesh && go->meshPath == libraryPath)
            count++;

        // Check texture
        if (go->texture && go->texture->texturePath == libraryPath)
            count++;
    }

    return count;
}

void AssetsWindow::BeginDragDropSource(const AssetInfo& asset)
{
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        // Payload type based on asset type
        const char* payloadType = "ASSET_DRAG";

        if (asset.type == AssetType::MESH)
            payloadType = "MESH_ASSET";
        else if (asset.type == AssetType::TEXTURE)
            payloadType = "TEXTURE_ASSET";
        else if (asset.type == AssetType::SCENE)
            payloadType = "SCENE_ASSET";

        ImGui::SetDragDropPayload(payloadType, asset.path.c_str(), asset.path.size() + 1);
        ImGui::Text("%s %s", GetAssetTypeIcon(asset.type), asset.name.c_str());
        ImGui::EndDragDropSource();
    }
}

AssetType AssetsWindow::GetAssetTypeFromExtension(const std::string& extension)
{
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".ilmesh")
        return AssetType::MESH;
    else if (ext == ".iltex")
        return AssetType::TEXTURE;
    else if (ext == ".ilscene")
        return AssetType::SCENE;

    return AssetType::UNKNOWN;
}

ImVec4 AssetsWindow::GetAssetTypeColor(AssetType type)
{
    switch (type)
    {
    case AssetType::FOLDER:
        return ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
    case AssetType::MESH:
        return ImVec4(0.2f, 0.8f, 1.0f, 1.0f);
    case AssetType::TEXTURE:
        return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
    case AssetType::SCENE:
        return ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
    default:
        return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    }
}

std::string AssetsWindow::FormatFileSize(uint64_t bytes)
{
    const char* units[] = { "B", "KB", "MB", "GB" };
    int unitIndex = 0;
    double size = (double)bytes;

    while (size >= 1024.0 && unitIndex < 3)
    {
        size /= 1024.0;
        unitIndex++;
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unitIndex]);
    return std::string(buffer);
}

const char* AssetsWindow::GetAssetTypeIcon(AssetType type)
{
    switch (type)
    {
    case AssetType::FOLDER:   return "[DIR]";
    case AssetType::MESH:     return "[MESH]";
    case AssetType::TEXTURE:  return "[TEX]";
    case AssetType::SCENE:    return "[SCN]";
    default:                  return "[?]";
    }
}