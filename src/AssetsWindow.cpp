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
#include "Ray.h"
#include "ModuleMousePicking.h"
#include "Camera.h"
#include <algorithm>
#include <functional>

AssetsWindow::AssetsWindow(ModuleEditor* editor)
    : EditorWindow(editor, "Assets")
    , assetsRoot("Assets")
    , currentPath("Assets")
    , gridView(true)
    , thumbnailSize(80.0f)
    , padding(16.0f)
    , timeSinceLastCheck(0.0f)
    , checkInterval(1.0f)
    , selectedIndex(-1)
{
    memset(searchBuffer, 0, sizeof(searchBuffer));

    // Create Assets structure if it doesn't exist
    if (!fs::exists(assetsRoot))
    {
        LOG("Assets folder does not exist - creating it");
        fs::create_directories(assetsRoot);
        fs::create_directories(assetsRoot + "/Models");
        fs::create_directories(assetsRoot + "/Textures");
        fs::create_directories(assetsRoot + "/Scenes");
    }

    if (fs::exists(assetsRoot) && fs::is_directory(assetsRoot))
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
}
void AssetsWindow::Draw()
{
    if (!visible) return;

    Window* window = Application::GetInstance().window.get();
    int windowWidth, windowHeight;
    window->GetWindowSize(windowWidth, windowHeight);

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

    timeSinceLastCheck += ImGui::GetIO().DeltaTime;
    if (timeSinceLastCheck >= checkInterval)
    {
        CheckForChanges();
        timeSinceLastCheck = 0.0f;
    }

    DrawToolbar();
    ImGui::Separator();

    float treeWidth = 200.0f;

    ImGui::BeginChild("FolderTree", ImVec2(treeWidth, -25), true);
    DrawFolderTree();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("AssetView", ImVec2(0, -25), true);

    DrawNavigationBar();
    ImGui::Separator();

    if (gridView)
    {
        DrawAssetGrid();
    }
    else
    {
        DrawAssetList();
    }

    ImGui::EndChild();

	HandleFileDrop();

    ImGui::Separator();
    int inMemoryCount = std::count_if(currentAssets.begin(), currentAssets.end(),
        [](const AssetInfo& a) { return a.isInMemory; });
    ImGui::Text("Assets: %d | In Memory: %d", (int)currentAssets.size(), inMemoryCount);

    ImGui::End();
}

void AssetsWindow::DrawToolbar()
{
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

    if (ImGui::Button(gridView ? "List" : "Grid"))
    {
        gridView = !gridView;
    }

    ImGui::SameLine();

    ImGui::PushItemWidth(200.0f);
    if (ImGui::InputTextWithHint("##search", "Search...", searchBuffer, sizeof(searchBuffer)))
    {
        RefreshCurrentFolder();
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();

    if (gridView)
    {
        ImGui::PushItemWidth(100.0f);
        ImGui::SliderFloat("##size", &thumbnailSize, 50.0f, 150.0f, "Size");
        ImGui::PopItemWidth();
    }
}

void AssetsWindow::DrawNavigationBar()
{
    std::string displayPath = currentPath;
    std::replace(displayPath.begin(), displayPath.end(), '\\', '/');
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Current: %s", displayPath.c_str());
}

void AssetsWindow::DrawFolderTree()
{
    if (!fs::exists(assetsRoot))
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Assets folder not found");
        return;
    }

    std::function<void(const std::string&, int)> drawNode = [&](const std::string& path, int depth)
        {
            if (path.empty() || !fs::exists(path) || !fs::is_directory(path))
                return;

            std::string folderName = fs::path(path).filename().string();
            if (folderName.empty())
                folderName = "Assets";

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

            if (path == currentPath)
                flags |= ImGuiTreeNodeFlags_Selected;

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
            catch (const std::exception&)
            {
                return;
            }

            if (!hasSubfolders)
                flags |= ImGuiTreeNodeFlags_Leaf;

            std::hash<std::string> hasher;
            size_t pathHash = hasher(path);
            ImGui::PushID((int)pathHash);

            std::string label = std::string(GetAssetTypeIcon(AssetType::FOLDER)) + " " + folderName;
            bool nodeOpen = ImGui::TreeNodeEx("##treenode", flags, "%s", label.c_str());

            if (ImGui::IsItemClicked())
            {
                NavigateToFolder(path);
            }

            if (nodeOpen)
            {
                try
                {
                    std::vector<std::string> subfolders;
                    for (const auto& entry : fs::directory_iterator(path))
                    {
                        if (entry.is_directory())
                        {
                            subfolders.push_back(entry.path().string());
                        }
                    }

                    std::sort(subfolders.begin(), subfolders.end());

                    for (const auto& subfolder : subfolders)
                    {
                        drawNode(subfolder, depth + 1);
                    }
                }
                catch (const std::exception&)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error reading folder");
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        };

    try
    {
        drawNode(assetsRoot, 0);
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

        ImGui::PushID(i + 1000);
        ImGui::BeginGroup();

        ImVec4 iconColor = GetAssetTypeColor(asset.type);

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

        std::string buttonLabel = std::string(GetAssetTypeIcon(asset.type)) + "##btn";
        bool clicked = ImGui::Button(buttonLabel.c_str(), ImVec2(thumbnailSize, thumbnailSize));

        ImGui::PopStyleColor(3);

        if (clicked)
        {
            selectedIndex = i;
        }

        // Double-click handler
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            HandleAssetDoubleClick(asset);
        }

        // Drag source for assets
        if (!asset.isDirectory)
        {
            BeginDragDropSource(asset);
        }

        float textWidth = thumbnailSize;
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + textWidth);

        std::string displayName = asset.name;
        if (displayName.length() > 20)
        {
            displayName = displayName.substr(0, 17) + "...";
        }

        ImGui::TextWrapped("%s", displayName.c_str());
        ImGui::PopTextWrapPos();

        if (asset.isInMemory && asset.referenceCount > 0)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Refs: %d", asset.referenceCount);
        }

        ImGui::EndGroup();

        std::string popupID = "ctx##" + std::to_string(i);
        if (ImGui::BeginPopupContextItem(popupID.c_str()))
        {
            DrawAssetContextMenu(asset);
            ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Name: %s", asset.name.c_str());
            if (!asset.isDirectory)
            {
                ImGui::Text("Size: %s", FormatFileSize(asset.fileSize).c_str());
                ImGui::Text("Type: %s",
                    asset.type == AssetType::MESH ? "Mesh" :
                    asset.type == AssetType::TEXTURE ? "Texture" :
                    asset.type == AssetType::SCENE ? "Scene" :
                    asset.type == AssetType::MODEL_SOURCE ? "Model Source" :
                    asset.type == AssetType::TEXTURE_SOURCE ? "Texture Source" : "Unknown");
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
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Refs", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < (int)currentAssets.size(); ++i)
        {
            const AssetInfo& asset = currentAssets[i];

            ImGui::TableNextRow();
            ImGui::PushID(i + 2000);

            ImGui::TableNextColumn();

            ImVec4 color = GetAssetTypeColor(asset.type);
            bool isSelected = (i == selectedIndex);
            std::string selectableID = "##sel" + std::to_string(i);

            if (ImGui::Selectable(selectableID.c_str(), isSelected,
                ImGuiSelectableFlags_SpanAllColumns |
                ImGuiSelectableFlags_AllowItemOverlap))
            {
                selectedIndex = i;
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                if (asset.isDirectory)
                {
                    NavigateToFolder(asset.path);
                }
                else if (asset.type == AssetType::SCENE)
                {
                    HandleSceneFileDrop(asset.path);
                }
            }

            ImGui::SameLine();
            ImGui::TextColored(color, "%s %s", GetAssetTypeIcon(asset.type), asset.name.c_str());

            if (!asset.isDirectory)
            {
                BeginDragDropSource(asset);
            }

            ImGui::TableNextColumn();
            if (asset.isDirectory)
            {
                ImGui::Text("Folder");
            }
            else
            {
                const char* typeStr =
                    asset.type == AssetType::MESH ? "Mesh" :
                    asset.type == AssetType::TEXTURE ? "Texture" :
                    asset.type == AssetType::SCENE ? "Scene" :
                    asset.type == AssetType::MODEL_SOURCE ? "Model Source" :
                    asset.type == AssetType::TEXTURE_SOURCE ? "Texture Source" : "Unknown";
                ImGui::Text("%s", typeStr);
            }

            ImGui::TableNextColumn();
            if (!asset.isDirectory)
            {
                ImGui::Text("%s", FormatFileSize(asset.fileSize).c_str());
            }
            else
            {
                ImGui::Text("-");
            }

            ImGui::TableNextColumn();
            if (asset.isInMemory && asset.referenceCount > 0)
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%d", asset.referenceCount);
            }
            else
            {
                ImGui::Text("-");
            }

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

    if (currentPath.empty())
    {
        currentPath = assetsRoot;
    }

    if (!fs::exists(currentPath))
    {
        LOG_WARNING("Current path does not exist: " + currentPath);
        return;
    }

    if (!fs::is_directory(currentPath))
    {
        LOG_WARNING("Current path is not a directory: " + currentPath);
        currentPath = assetsRoot;
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

                if (info.name.empty())
                    continue;

                info.isDirectory = entry.is_directory();

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

                    info.referenceCount = GetReferenceCount(info.path);
                    info.isInMemory = (info.referenceCount > 0);

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
            catch (const std::exception&)
            {
                continue;
            }
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error scanning folder '" + currentPath + "': " + std::string(e.what()));
        return;
    }

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
        for (const auto& entry : fs::directory_iterator(currentPath))
        {
            if (entry.is_directory())
                continue;

            std::string path = entry.path().string();
            auto newTime = fs::last_write_time(entry.path());

            auto it = fileTimestamps.find(path);
            if (it == fileTimestamps.end())
            {
                LOG("New asset detected: " + path);
                needsRefresh = true;
            }
            else if (it->second != newTime)
            {
                LOG("Asset modified: " + path);
                fileTimestamps[path] = newTime;
                needsRefresh = true;
            }
        }

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

        // Don't go above Assets root
        if (parent.string().length() >= assetsRoot.length())
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
    LOG("Importing dropped file: " + filePath);

    std::string extension = fs::path(filePath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    std::string targetDir = currentPath;
    std::string fileName = fs::path(filePath).filename().string();
    std::string targetPath = targetDir + "/" + fileName;

    try
    {
        // Handle model files
        if (IsModelFile(extension))
        {
            // Ensure we're in Models folder or navigate there
            if (currentPath.find("Models") == std::string::npos)
            {
                targetDir = assetsRoot + "/Models";
                targetPath = targetDir + "/" + fileName;
            }

            if (!fs::exists(targetDir))
            {
                fs::create_directories(targetDir);
            }

            fs::copy_file(filePath, targetPath, fs::copy_options::overwrite_existing);
            LOG("Model file copied to: " + targetPath);
            RefreshCurrentFolder();
            return true;
        }
        // Handle texture files
        else if (IsTextureFile(extension))
        {
            // Ensure we're in Textures folder or navigate there
            if (currentPath.find("Textures") == std::string::npos)
            {
                targetDir = assetsRoot + "/Textures";
                targetPath = targetDir + "/" + fileName;
            }

            if (!fs::exists(targetDir))
            {
                fs::create_directories(targetDir);
            }

            fs::copy_file(filePath, targetPath, fs::copy_options::overwrite_existing);
            LOG("Texture file copied to: " + targetPath);
            RefreshCurrentFolder();
            return true;
        }
        // Handle scene files
        else if (IsSceneFile(extension))
        {
            // Ensure we're in Scenes folder or navigate there
            if (currentPath.find("Scenes") == std::string::npos)
            {
                targetDir = assetsRoot + "/Scenes";
                targetPath = targetDir + "/" + fileName;
            }

            if (!fs::exists(targetDir))
            {
                fs::create_directories(targetDir);
            }

            fs::copy_file(filePath, targetPath, fs::copy_options::overwrite_existing);
            LOG("Scene file copied to: " + targetPath);
            RefreshCurrentFolder();
            return true;
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to copy file: " + std::string(e.what()));
        return false;
    }

    LOG_WARNING("Unknown file format: " + extension);
    return false;
}

int AssetsWindow::GetReferenceCount(const std::string& path)
{
    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (!opengl)
        return 0;

    int count = 0;

    for (GameObject* go : opengl->gameObjects)
    {
        if (!go)
            continue;

        if (go->mesh && go->meshPath == path)
            count++;

        if (go->texture && go->texture->texturePath == path)
            count++;
    }

    return count;
}

void AssetsWindow::BeginDragDropSource(const AssetInfo& asset)
{
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        const char* payloadType = "ASSET_FILE";

        if (asset.type == AssetType::MESH || asset.type == AssetType::MODEL_SOURCE)
        {
            payloadType = "MESH_FILE";
            ImGui::SetDragDropPayload(payloadType, asset.path.c_str(), asset.path.size() + 1);
        }
        else if (asset.type == AssetType::TEXTURE || asset.type == AssetType::TEXTURE_SOURCE)
        {
            payloadType = "TEXTURE_FILE";
            ImGui::SetDragDropPayload(payloadType, asset.path.c_str(), asset.path.size() + 1);
        }
        else if (asset.type == AssetType::SCENE)
        {
            payloadType = "SCENE_FILE";
            ImGui::SetDragDropPayload(payloadType, asset.path.c_str(), asset.path.size() + 1);
        }

        ImGui::Text("%s %s", GetAssetTypeIcon(asset.type), asset.name.c_str());
        ImGui::EndDragDropSource();
    }
}

void AssetsWindow::HandleSceneFileDrop(const std::string& scenePath)
{
    if (editor->sceneModified)
    {
        LOG_WARNING("Current scene has unsaved changes!");
    }

    editor->LoadScene(scenePath);
    LOG("Loaded scene from assets: " + scenePath);
}

void AssetsWindow::HandleMeshFileDrop(const std::string& meshPath, float mouseX, float mouseY)
{
    LOG("Mesh drop from assets: " + meshPath);
}

void AssetsWindow::HandleTextureFileDrop(const std::string& texturePath)
{
    LOG("Texture drop from assets: " + texturePath);
}

AssetType AssetsWindow::GetAssetTypeFromExtension(const std::string& extension)
{
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Custom formats
    if (ext == ".ilmesh")
        return AssetType::MESH;
    else if (ext == ".iltex")
        return AssetType::TEXTURE;
    else if (ext == ".ilscene")
        return AssetType::SCENE;
    // Source model formats
    else if (ext == ".fbx" || ext == ".obj" || ext == ".dae" || ext == ".gltf" || ext == ".glb")
        return AssetType::MODEL_SOURCE;
    // Source texture formats
    else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
        ext == ".tga" || ext == ".dds" || ext == ".hdr")
        return AssetType::TEXTURE_SOURCE;

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
    case AssetType::MODEL_SOURCE:
        return ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
    case AssetType::TEXTURE_SOURCE:
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
    case AssetType::MODEL_SOURCE:    return "[MODEL_SC]";
    case AssetType::TEXTURE_SOURCE:    return "[TEX_SC]";
    default:                  return "[?]";
    }
}

void AssetsWindow::HandleAssetDoubleClick(const AssetInfo& asset)
{
    if (asset.isDirectory)
    {
        NavigateToFolder(asset.path);
        return;
    }

    // Handle scene files
    if (asset.type == AssetType::SCENE)
    {
        if (editor->sceneModified)
        {
            LOG_WARNING("Current scene has unsaved changes!");
        }

        editor->LoadScene(asset.path);
        LOG("Loaded scene from assets: " + asset.path);
        return;
    }

    LOG("Double-clicked asset: " + asset.name);
}

bool AssetsWindow::IsModelFile(const std::string& extension)
{
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return (ext == ".fbx" || ext == ".obj" || ext == ".dae" ||
        ext == ".gltf" || ext == ".glb");
}

bool AssetsWindow::IsTextureFile(const std::string& extension)
{
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return (ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
        ext == ".bmp" || ext == ".tga" || ext == ".dds" || ext == ".hdr");
}

bool AssetsWindow::IsSceneFile(const std::string& extension)
{
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return (ext == ".ilscene");
}

void AssetsWindow::HandleFileDrop()
{
    // Create an invisible window that covers the entire Assets area to capture drops
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();

    // Check if there is an active drag
    if (ImGui::BeginDragDropTarget())
    {
        // Try to accept files from the system
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SDL_DROP_FILE"))
        {
            // This payload will come from the system when a file is dropped
            const char* droppedPath = (const char*)payload->Data;
            if (droppedPath && strlen(droppedPath) > 0)
            {
                std::string filePath = droppedPath;
                LOG("File dropped on Assets window: " + filePath);

                // Copy the file to the current folder
                if (ImportDroppedFile(filePath))
                {
                    LOG("File successfully imported to Assets folder");
                    RefreshCurrentFolder();
                }
            }
        }

        ImGui::EndDragDropTarget();
    }
}
