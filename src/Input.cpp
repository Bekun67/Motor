#include "Input.h"
#include "Window.h"
#include "Application.h"
#include "AssetsWindow.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "imgui_impl_sdl3.h"
#include <imgui.h>     
#include <ImGuizmo.h>  
#include "EditorPlaySystem.h"
#include <functional>
#include <filesystem>
namespace fs = std::filesystem;

#define MAX_KEYS 300

Input::Input() : Module()
{
    keyboard = new KeyState[MAX_KEYS];
    memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
    memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);
}

Input::~Input()
{
    delete[] keyboard;
}

bool Input::Awake()
{
    bool ret = true;
    SDL_Init(0);

    if (SDL_InitSubSystem(SDL_INIT_EVENTS) < 0)
    {
        ret = false;
    }

    return ret;
}

bool Input::Start()
{
    return true;
}

bool Input::PreUpdate()
{
    static SDL_Event event;

    mouseWheelX = 0;
    mouseWheelY = 0;

    const bool* keys = SDL_GetKeyboardState(NULL);
    for (int i = 0; i < MAX_KEYS; ++i)
    {
        if (keys[i])
        {
            if (keyboard[i] == KEY_IDLE)
                keyboard[i] = KEY_DOWN;
            else
                keyboard[i] = KEY_REPEAT;
        }
        else
        {
            if (keyboard[i] == KEY_REPEAT || keyboard[i] == KEY_DOWN)
                keyboard[i] = KEY_UP;
            else
                keyboard[i] = KEY_IDLE;
        }
    }

    for (int i = 0; i < NUM_MOUSE_BUTTONS; ++i)
    {
        if (mouseButtons[i] == KEY_DOWN)
            mouseButtons[i] = KEY_REPEAT;
        if (mouseButtons[i] == KEY_UP)
            mouseButtons[i] = KEY_IDLE;
    }
    ModuleEditor* moduleEditor = Application::GetInstance().editor.get();

    OpenGL* opengl = Application::GetInstance().opengl.get();
    float scaleFactor = 0.1f;
    Application& app = Application::GetInstance();
    ModuleEditor* editor = app.editor.get();

    if (!moduleEditor->editing) {
        // GameObject deletion with DELETE key
        if (keyboard[SDL_SCANCODE_DELETE] == KEY_DOWN) {
            if (!editor->selectedGameObjects.empty()) {
                LOG("Deleting " + std::to_string(editor->selectedGameObjects.size()) + " GameObject(s)");

                std::vector<GameObject*> toDelete = editor->selectedGameObjects;

                // Deselect all first
                editor->DeselectAll();

                // filter only roots
                std::vector<GameObject*> rootsToDelete;
                for (GameObject* go : toDelete)
                {
                    // see if it stil exists
                    auto findIt = std::find(opengl->gameObjects.begin(), opengl->gameObjects.end(), go);
                    if (findIt == opengl->gameObjects.end())
                    {
                        continue;
                    }

                    // only add if it isn't another children
                    bool isChildOfOtherSelected = false;
                    for (GameObject* other : toDelete)
                    {
                        if (other != go && go->IsDescendantOf(other))
                        {
                            isChildOfOtherSelected = true;
                            break;
                        }
                    }

                    if (!isChildOfOtherSelected)
                    {
                        rootsToDelete.push_back(go);
                    }
                }

                // delete every root with descendants
                for (GameObject* go : rootsToDelete)
                {
                    auto findIt = std::find(opengl->gameObjects.begin(), opengl->gameObjects.end(), go);
                    if (findIt == opengl->gameObjects.end())
                    {
                        continue;
                    }

                    // recolect all descendants
                    std::vector<GameObject*> allObjects;
                    allObjects.push_back(go);
                    go->GetAllDescendants(allObjects);

                    for (GameObject* obj : allObjects)
                    {
                        auto it = std::find(opengl->gameObjects.begin(), opengl->gameObjects.end(), obj);
                        if (it != opengl->gameObjects.end())
                        {
                            opengl->gameObjects.erase(it);
                        }
                    }
                    delete go;
                }

                editor->sceneModified = true;
                if (opengl->useQuadtree) {
                    if (opengl->EmptyQuadtree()) {
                        LOG("Quadtree is now empty");
                    }
                    else {
                        opengl->RebuildQuadtree();
                    }
                }
            }
        }

        // GameObject selection with F1/F2
        if (keyboard[SDL_SCANCODE_F1] == KEY_DOWN || keyboard[SDL_SCANCODE_F2] == KEY_DOWN)
        {
            int index = -1;
            if (opengl->gameObjects.size() > 0) {
                //list with hierarchy order
                std::vector<GameObject*> flatHierarchy;

                std::vector<GameObject*> roots;
                for (GameObject* go : opengl->gameObjects)
                {
                    if (go != nullptr && go->parent == nullptr)
                    {
                        roots.push_back(go);
                    }
                }

                std::function<void(GameObject*)> addWithChildren = [&](GameObject* go)
                    {
                        if (go == nullptr) return;
                        flatHierarchy.push_back(go);
                        for (GameObject* child : go->children)
                        {
                            addWithChildren(child);
                        }
                    };

                for (GameObject* root : roots)
                {
                    addWithChildren(root);
                }

                if (flatHierarchy.empty())
                {
                    //no game objects
                    LOG("No Game Objects in scene to select");
                }
                else
                {
                    GameObject* currentSelection = editor->selectedGameObjects.empty() ? nullptr : editor->selectedGameObjects[0];

                    for (int i = 0; i < flatHierarchy.size(); i++)
                    {
                        if (currentSelection == flatHierarchy[i]) index = i;
                    }

                    if (keyboard[SDL_SCANCODE_F2] == KEY_DOWN && index < flatHierarchy.size() - 1) {
                        GameObject* nextObject = flatHierarchy[index + 1];
                        editor->SelectGameObject(nextObject, false);
                        opengl->selectedGameObject = nextObject;
                        LOG("Selecting next Game Object, " + nextObject->name);
                    }
                    if (keyboard[SDL_SCANCODE_F1] == KEY_DOWN && index > 0) {
                        GameObject* prevObject = flatHierarchy[index - 1];
                        editor->SelectGameObject(prevObject, false);
                        opengl->selectedGameObject = prevObject;
                        LOG("Selecting previous Game Object, " + prevObject->name);
                    }
                }
            }
            else
            {
                //no game objects
                LOG("No Game Objects in scene to select");
            }
        }

        // Deselect with F3
        if (keyboard[SDL_SCANCODE_F3] == KEY_DOWN && !editor->selectedGameObjects.empty())
        {
            LOG("Deselecting all GameObjects");
            editor->DeselectAll();
            opengl->selectedGameObject = nullptr;
        }
    }

    if (!moduleEditor->editing)
    {
        // F5 - Play
        if (keyboard[SDL_SCANCODE_F5] == KEY_DOWN)
        {
            if (EditorPlaySystem::IsStopped() || EditorPlaySystem::IsPaused())
            {
                EditorPlaySystem::Play();
            }
        }

        // F6 - Pause
        if (keyboard[SDL_SCANCODE_F6] == KEY_DOWN)
        {
            if (EditorPlaySystem::IsPlaying())
            {
                EditorPlaySystem::Pause();
            }
        }

        // F7 - Stop
        if (keyboard[SDL_SCANCODE_F7] == KEY_DOWN)
        {
            if (EditorPlaySystem::IsPlaying())
            {
                EditorPlaySystem::Stop();
            }
        }
    }

    while (SDL_PollEvent(&event))
    {

        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            windowEvents[WE_QUIT] = true;
            break;
        case SDL_EVENT_WINDOW_HIDDEN:
        case SDL_EVENT_WINDOW_MINIMIZED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            windowEvents[WE_HIDE] = true;
            break;
        case SDL_EVENT_WINDOW_SHOWN:
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_MAXIMIZED:
        case SDL_EVENT_WINDOW_RESTORED:
            windowEvents[WE_SHOW] = true;
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            mouseButtons[event.button.button - 1] = KEY_DOWN;
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            mouseButtons[event.button.button - 1] = KEY_UP;
            break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            int scale = Application::GetInstance().window.get()->GetScale();
            mouseMotionX = event.motion.xrel / scale;
            mouseMotionY = event.motion.yrel / scale;
            mouseX = event.motion.x / scale;
            mouseY = event.motion.y / scale;
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL:
        {
            mouseWheelX = event.wheel.x;
            mouseWheelY = event.wheel.y;
            break;
        }
        case SDL_EVENT_DROP_FILE:
        {
            SDL_DropEvent drop = event.drop;
            const char* droppedFile = drop.data;

            float mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);

            // Check if mouse is inside different windows
            bool mouseInsideAssets = false;
            bool mouseInsideScene = false;
            bool mouseInsideTextureInspector = false;

            // Check Assets window
            if (moduleEditor->showAssets)
            {
                Window* window = Application::GetInstance().window.get();
                int windowWidth, windowHeight;
                window->GetWindowSize(windowWidth, windowHeight);

                float assetsMinX = windowWidth * moduleEditor->layout.consoleXPercent;
                float assetsMaxX = assetsMinX + (windowWidth * moduleEditor->layout.consoleWidthPercent);
                float assetsMinY = windowHeight * moduleEditor->layout.consoleYPercent;
                float assetsMaxY = assetsMinY + (windowHeight * moduleEditor->layout.consoleHeightPercent);

                if (mouseX >= assetsMinX && mouseX <= assetsMaxX &&
                    mouseY >= assetsMinY && mouseY <= assetsMaxY)
                {
                    mouseInsideAssets = true;
                }
            }

            // Check Scene viewport
            if (!mouseInsideAssets)
            {
                int minX = moduleEditor->sceneViewportPos.x;
                int maxX = moduleEditor->sceneViewportPos.x + moduleEditor->sceneViewportSize.x;
                int minY = moduleEditor->sceneViewportPos.y;
                int maxY = moduleEditor->sceneViewportPos.y + moduleEditor->sceneViewportSize.y;

                if (mouseX > minX && mouseX < maxX &&
                    mouseY > minY && mouseY < maxY)
                {
                    mouseInsideScene = true;
                }
            }

            // Check Inspector texture drop area
            if (!mouseInsideAssets && !mouseInsideScene && moduleEditor->showInspector)
            {
                if (!moduleEditor->selectedGameObjects.empty())
                {
                    if (mouseX >= moduleEditor->textureDropPos.x &&
                        mouseX <= moduleEditor->textureDropPos.x + moduleEditor->textureDropSize.x &&
                        mouseY >= moduleEditor->textureDropPos.y &&
                        mouseY <= moduleEditor->textureDropPos.y + moduleEditor->textureDropSize.y)
                    {
                        mouseInsideTextureInspector = true;
                    }
                }
            }

            // Assets Window: Import file to current Assets folder
            if (mouseInsideAssets && droppedFile)
            {
                std::string path(droppedFile);

                // Normalize path separators
                for (size_t i = 0; i < path.size(); ++i) {
                    if (path[i] == '\\') path[i] = '/';
                }

                LOG("File dropped on Assets window: " + path);

                // Get file extension
                std::string extension = "";
                size_t dotPos = path.find_last_of('.');
                if (dotPos != std::string::npos && dotPos < path.length() - 1) {
                    extension = path.substr(dotPos);
                    for (size_t i = 0; i < extension.size(); ++i) {
                        extension[i] = (char)tolower(extension[i]);
                    }
                }

                // Determine target directory based on file type
                std::string targetDir = static_cast<AssetsWindow*>(moduleEditor->assetsWindow.get())->currentPath;

                // If we're in the root Assets folder, use appropriate subfolder
                if (targetDir == "Assets")
                {
                    if (extension == ".fbx" || extension == ".obj" ||
                        extension == ".dae" || extension == ".gltf" || extension == ".glb")
                    {
                        targetDir = "Assets/Models";
                    }
                    else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
                        extension == ".bmp" || extension == ".tga" || extension == ".dds" || extension == ".hdr")
                    {
                        targetDir = "Assets/Textures";
                    }
                    else if (extension == ".ilscene")
                    {
                        targetDir = "Assets/Scenes";
                    }
                }

                // Create directory if it doesn't exist
                if (!fs::exists(targetDir))
                {
                    fs::create_directories(targetDir);
                }

                std::string fileName = fs::path(path).filename().string();
                std::string targetPath = targetDir + "/" + fileName;

                try
                {
                    // Copy file to target directory
                    fs::copy_file(path, targetPath, fs::copy_options::overwrite_existing);
                    LOG("File copied to: " + targetPath);

                    // Refresh Assets window view
                    if (moduleEditor->assetsWindow)
                    {
                        AssetsWindow* assetsWindow = static_cast<AssetsWindow*>(moduleEditor->assetsWindow.get());

                        // Navigate to the folder where file was copied
                        assetsWindow->NavigateToFolder(targetDir);
                        assetsWindow->RefreshCurrentFolder();
                    }
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR("Failed to copy file: " + std::string(e.what()));
                }

                break; // Stop processing this event
            }

            // Scene Viewport: Load mesh into scene at mouse position
            if (mouseInsideScene && droppedFile)
            {
                std::string path(droppedFile);

                // Normalize path
                for (size_t i = 0; i < path.size(); ++i) {
                    if (path[i] == '\\') path[i] = '/';
                }

                std::cout << "========================" << std::endl;
                std::cout << "Dropped file: " << path << std::endl;

                // Get file extension
                std::string extension = "";
                size_t dotPos = path.find_last_of('.');
                if (dotPos != std::string::npos && dotPos < path.length() - 1) {
                    extension = path.substr(dotPos);
                    // Convert to lowercase
                    for (size_t i = 0; i < extension.size(); ++i) {
                        extension[i] = (char)tolower(extension[i]);
                    }
                }

                if (extension == ".fbx") {
                    if (!mouseInsideScene)
                    {
                        LOG_WARNING("Drop mesh in scene");
                        break;
                    }

                    std::cout << "=========MESH===========" << std::endl;

                    size_t meshCountBefore = g_Meshes.size();
                    size_t instanceCountBefore = g_MeshInstances.size();

                    if (LoadFile(path.c_str())) {
                        std::cout << "FBX loaded" << std::endl;

                        size_t newInstanceCount = g_MeshInstances.size() - instanceCountBefore;

                        // Get mouse position
                        float mouseX, mouseY;
                        SDL_GetMouseState(&mouseX, &mouseY);

                        float relativeMouseX = mouseX - moduleEditor->sceneViewportPos.x;
                        float relativeMouseY = mouseY - moduleEditor->sceneViewportPos.y;

                        // Get camera
                        Camera* camera = &(Application::GetInstance().opengl->camera);

                        int viewportWidth = (int)moduleEditor->sceneViewportSize.x;
                        int viewportHeight = (int)moduleEditor->sceneViewportSize.y;

                        // Convert coordinates using relative coordinates
                        float x = (2.0f * relativeMouseX) / viewportWidth - 1.0f;
                        float y = 1.0f - (2.0f * relativeMouseY) / viewportHeight;

                        // Calculate ray
                        glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
                        glm::vec4 rayEye = glm::inverse(camera->GetProjectionMatrix()) * rayClip;
                        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
                        glm::vec3 rayWorld = glm::vec3(glm::inverse(camera->GetViewMatrix()) * rayEye);
                        rayWorld = glm::normalize(rayWorld);

                        // Intersect with floor
                        glm::vec3 camPos = camera->GetPosition();
                        float t = -camPos.y / rayWorld.y;
                        glm::vec3 dropPosition = camPos + rayWorld * t;

                        // Calculate model size for normalization
                        glm::vec3 globalMin(FLT_MAX);
                        glm::vec3 globalMax(-FLT_MAX);

                        for (size_t i = instanceCountBefore; i < g_MeshInstances.size(); ++i)
                        {
                            const MeshWithTransform& inst = g_MeshInstances[i];
                            int meshIdx = inst.meshIndex;

                            if (meshIdx >= 0 && meshIdx < (int)g_Meshes.size())
                            {
                                const MeshData& meshData = g_Meshes[meshIdx];

                                glm::vec3 corners[8] = {
                                    glm::vec3(meshData.aabbMin.x, meshData.aabbMin.y, meshData.aabbMin.z),
                                    glm::vec3(meshData.aabbMax.x, meshData.aabbMin.y, meshData.aabbMin.z),
                                    glm::vec3(meshData.aabbMax.x, meshData.aabbMax.y, meshData.aabbMin.z),
                                    glm::vec3(meshData.aabbMin.x, meshData.aabbMax.y, meshData.aabbMin.z),
                                    glm::vec3(meshData.aabbMin.x, meshData.aabbMin.y, meshData.aabbMax.z),
                                    glm::vec3(meshData.aabbMax.x, meshData.aabbMin.y, meshData.aabbMax.z),
                                    glm::vec3(meshData.aabbMax.x, meshData.aabbMax.y, meshData.aabbMax.z),
                                    glm::vec3(meshData.aabbMin.x, meshData.aabbMax.y, meshData.aabbMax.z)
                                };

                                for (int c = 0; c < 8; ++c)
                                {
                                    glm::vec4 worldCorner = inst.transform * glm::vec4(corners[c], 1.0f);
                                    glm::vec3 corner3 = glm::vec3(worldCorner);

                                    globalMin = glm::vec3(std::min(globalMin.x, corner3.x),
                                        std::min(globalMin.y, corner3.y),
                                        std::min(globalMin.z, corner3.z));
                                    globalMax = glm::vec3(std::max(globalMax.x, corner3.x),
                                        std::max(globalMax.y, corner3.y),
                                        std::max(globalMax.z, corner3.z));
                                }
                            }
                        }

                        float modelSize = glm::length(globalMax - globalMin);
                        float targetSize = 2.0f;
                        float normalizeScale = (modelSize > 0.0001f) ? (targetSize / modelSize) : 1.0f;

                        // Calculate global minimum Y from ALL geometry
                        float globalMinY = FLT_MAX;

                        for (size_t i = instanceCountBefore; i < g_MeshInstances.size(); ++i)
                        {
                            const MeshWithTransform& inst = g_MeshInstances[i];
                            int meshIdx = inst.meshIndex;

                            if (meshIdx >= 0 && meshIdx < (int)g_Meshes.size()) {
                                const MeshData& meshData = g_Meshes[meshIdx];

                                glm::vec4 worldMin = inst.transform * glm::vec4(meshData.aabbMin, 1.0f);
                                globalMinY = std::min(globalMinY, worldMin.y);
                            }
                        }

                        // Count unique meshes
                        Assimp::Importer counter;
                        const aiScene* countScene = counter.ReadFile(path.c_str(), aiProcess_Triangulate);
                        int numMeshesInFBX = countScene ? countScene->mNumMeshes : 2;

                        // Game object for each model
                        std::vector<GameObject*> createdObjects;
                        int baseIndex = moduleEditor->CountNames("DroppedMesh_");

                        for (size_t i = instanceCountBefore; i < g_MeshInstances.size(); ++i)
                        {
                            // Create gameobject with mesh
                            const MeshWithTransform& inst = g_MeshInstances[i];
                            int meshIdx = inst.meshIndex;

                            if (meshIdx < 0 || meshIdx >= (int)g_Meshes.size()) {
                                continue;
                            }

                            MeshData& meshData = g_Meshes[meshIdx];

                            glm::vec3 meshLocalCenter = (meshData.aabbMin + meshData.aabbMax) * 0.5f;

                            glBindBuffer(GL_ARRAY_BUFFER, meshData.VBO);
                            GLint bufferSize;
                            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);

                            int vertexSize = 8;
                            int numVertices = bufferSize / (vertexSize * sizeof(float));

                            std::vector<float> vertexData(bufferSize / sizeof(float));
                            glGetBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, vertexData.data());

                            for (int v = 0; v < numVertices; ++v) {
                                int offset = v * vertexSize;
                                vertexData[offset + 0] -= meshLocalCenter.x;
                                vertexData[offset + 1] -= meshLocalCenter.y;
                                vertexData[offset + 2] -= meshLocalCenter.z;
                            }

                            glBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, vertexData.data());
                            glBindBuffer(GL_ARRAY_BUFFER, 0);

                            meshData.aabbMin -= meshLocalCenter;
                            meshData.aabbMax -= meshLocalCenter;
                            meshData.center = glm::vec3(0, 0, 0);

                            // Create game object
                            GameObject* go = new GameObject();
                            int index = moduleEditor->CountNames("DroppedMesh_");
                            go->name = "DroppedMesh_" + std::to_string(index);
                            go->meshPath = path;

                            go->meshIndexInFBX = (i - instanceCountBefore) % numMeshesInFBX;

                            go->mesh->meshIndex = meshIdx;

                            glm::vec3 instancePosition, instanceScale;
                            glm::quat instanceRotation;
                            DecomposeTransform(inst.transform, instancePosition, instanceRotation, instanceScale);

                            glm::vec4 meshWorldCenter4 = inst.transform * glm::vec4(meshLocalCenter, 1.0f);
                            glm::vec3 meshWorldCenter = glm::vec3(meshWorldCenter4);

                            glm::vec3 finalPos;
                            finalPos.x = dropPosition.x + (meshWorldCenter.x - g_ModelCenter.x) * normalizeScale;
                            finalPos.z = dropPosition.z + (meshWorldCenter.z - g_ModelCenter.z) * normalizeScale;
                            finalPos.y = (meshWorldCenter.y - globalMinY) * normalizeScale;

                            // Set translation to match obtained coordinates
                            go->transform->translation = aiVector3D(finalPos.x, finalPos.y, finalPos.z);

                            // Set rotation to match obtained rotation
                            go->transform->rotation = aiQuaternion(
                                instanceRotation.w,
                                instanceRotation.x,
                                instanceRotation.y,
                                instanceRotation.z
                            );

                            // Set scale to match obtained normalized scale
                            go->transform->scaling = aiVector3D(
                                instanceScale.x * normalizeScale,
                                instanceScale.y * normalizeScale,
                                instanceScale.z * normalizeScale
                            );

                            // Try to load texture
                            std::string texturePath = GetTexturePathFromFBX(path.c_str(), go->meshIndexInFBX);

                            bool textureLoaded = false;
                            if (!texturePath.empty()) {
                                std::cout << "Assigned texture from FBX: " << texturePath << std::endl;
                                textureLoaded = go->texture->LoadTexture(texturePath);
                            }

                            if (!textureLoaded) {
                                // If no texture available we use checkerboard
                                std::cout << "No valid texture found, using checkerboard" << std::endl;

                                const int size = 64;
                                GLubyte checkerImage[64][64][4];
                                for (int i = 0; i < size; i++) {
                                    for (int j = 0; j < size; j++) {
                                        int c = ((((i & 0x8) == 0) ^ ((j & 0x8) == 0))) * 255;
                                        checkerImage[i][j][0] = (GLubyte)c;
                                        checkerImage[i][j][1] = (GLubyte)c;
                                        checkerImage[i][j][2] = (GLubyte)c;
                                        checkerImage[i][j][3] = (GLubyte)255;
                                    }
                                }

                                GLuint checkID;
                                glGenTextures(1, &checkID);
                                glBindTexture(GL_TEXTURE_2D, checkID);
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerImage);
                                glGenerateMipmap(GL_TEXTURE_2D);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                                glBindTexture(GL_TEXTURE_2D, 0);

                                go->texture->hasTexture = true;
                                if (go->texture->texturedata == nullptr) {
                                    go->texture->texturedata = new TextureData();
                                }
                                go->texture->texturedata->id = checkID;
                                go->texture->texturedata->type = "checkerboard";
                                go->texture->texturedata->path = "checkerboard";

                                std::cout << "Checkerboard created with ID: " << checkID << std::endl;
                            }

                            Application::GetInstance().opengl->gameObjects.push_back(go);
                            createdObjects.push_back(go);

                            std::cout << "Created GameObject " << go->name
                                << " at position (" << go->transform->translation.x
                                << ", " << go->transform->translation.y
                                << ", " << go->transform->translation.z << ")"
                                << std::endl;
                        }

                        if (!createdObjects.empty())
                        {
                            glm::vec3 groupCenter(0.0f);
                            for (GameObject* obj : createdObjects)
                            {
                                groupCenter.x += obj->transform->translation.x;
                                groupCenter.y += obj->transform->translation.y;
                                groupCenter.z += obj->transform->translation.z;
                            }
                            groupCenter /= (float)createdObjects.size();

                            GameObject* parentEmpty = new GameObject();
                            std::string fileName = fs::path(path).stem().string();
                            int parentIndex = moduleEditor->CountNames(fileName + "_");
                            parentEmpty->name = fileName + "_" + std::to_string(parentIndex);
                            parentEmpty->meshPath = "";
                            parentEmpty->meshIndexInFBX = -1;
                            parentEmpty->mesh->meshIndex = -1;

                            parentEmpty->transform->translation = aiVector3D(groupCenter.x, groupCenter.y, groupCenter.z);
                            parentEmpty->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
                            parentEmpty->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

                            Application::GetInstance().opengl->gameObjects.push_back(parentEmpty);

                            for (GameObject* child : createdObjects)
                            {
                                child->parent = parentEmpty;
                                parentEmpty->children.push_back(child);
                            }

                            // Logs
                            LOG("=== FBX Import (Drag & Drop) ===");
                            LOG("File: " + fileName);
                            LOG("Created parent: " + parentEmpty->name + " at position (" +
                                std::to_string(groupCenter.x) + ", " +
                                std::to_string(groupCenter.y) + ", " +
                                std::to_string(groupCenter.z) + ")");
                            LOG("Total meshes imported: " + std::to_string(createdObjects.size()));
                        }

                        ModuleEditor* editor = Application::GetInstance().editor.get();
                        if (editor) editor->sceneModified = true;

                        std::cout << "Total GameObjects created: " << createdObjects.size() << std::endl;
                        std::cout << "Total GameObjects in scene: " << Application::GetInstance().opengl->gameObjects.size() << std::endl;
                    }
                }

                else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
                    // Get mouse position
                    if (!mouseInsideScene && !mouseInsideTextureInspector)
                    {
                        if (!moduleEditor->selectedGameObjects.empty())
                            LOG_WARNING("Drop texture over a GameObject or in the Inspector tab!");
                        break;
                    }
                    std::cout << "========TEXTURE==========" << std::endl;
                    SDL_GetMouseState(&mouseX, &mouseY);

                    // Get camera
                    Camera* camera = &(Application::GetInstance().opengl->camera);

                    int viewport[4];
                    glGetIntegerv(GL_VIEWPORT, viewport);

                    // Convert coordinates
                    float x = (2.0f * mouseX) / viewport[2] - 1.0f;
                    float y = 1.0f - (2.0f * mouseY) / viewport[3];

                    // Create ray
                    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
                    glm::vec4 rayEye = glm::inverse(camera->GetProjectionMatrix()) * rayClip;
                    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
                    glm::vec3 rayWorld = glm::vec3(glm::inverse(camera->GetViewMatrix()) * rayEye);
                    rayWorld = glm::normalize(rayWorld);

                    glm::vec3 camPos = camera->GetPosition();

                    GameObject* closestObject = nullptr;

                    // If we are hovering inside the "Drag new texture here:" panel we change the selectedGameObject's texture
                    if (mouseInsideTextureInspector && !moduleEditor->selectedGameObjects.empty())
                        closestObject = moduleEditor->selectedGameObjects[0];

                    // If we are hovering over the scene we find the closest game object
                    else
                    {
                        float closestDistance = FLT_MAX;

                        // Get viewport bounds from editor
                        float viewportMinX = moduleEditor->sceneViewportPos.x;
                        float viewportMinY = moduleEditor->sceneViewportPos.y;
                        float viewportWidth = moduleEditor->sceneViewportSize.x;
                        float viewportHeight = moduleEditor->sceneViewportSize.y;

                        float relativeMouseX = mouseX - viewportMinX;
                        float relativeMouseY = mouseY - viewportMinY;

                        // Check if mouse is within viewport bounds
                        if (relativeMouseX >= 0 && relativeMouseX <= viewportWidth &&
                            relativeMouseY >= 0 && relativeMouseY <= viewportHeight)
                        {
                            ModuleMousePicking* mousePicking = Application::GetInstance().mousePicking.get();

                            // Create ray using viewport-relative coordinates
                            Ray ray = mousePicking->CreateRayFromMouse(
                                relativeMouseX,
                                relativeMouseY,
                                camera,
                                (int)viewportWidth,
                                (int)viewportHeight
                            );

                            // Test ray against all game objects using AABB
                            for (GameObject* go : Application::GetInstance().opengl->gameObjects) {
                                if (go->mesh->meshIndex < 0 || go->mesh->meshIndex >= (int)g_Meshes.size()) {
                                    continue;
                                }

                                MeshData& meshData = g_Meshes[go->mesh->meshIndex];
                                ComponentTransform* transform = go->transform;

                                glm::mat4 model = glm::mat4(1.0f);
                                model = glm::translate(model, glm::vec3(
                                    transform->translation.x,
                                    transform->translation.y,
                                    transform->translation.z
                                ));

                                glm::quat quat(
                                    transform->rotation.w,
                                    transform->rotation.x,
                                    transform->rotation.y,
                                    transform->rotation.z
                                );
                                model *= glm::mat4_cast(quat);

                                model = glm::scale(model, glm::vec3(
                                    transform->scaling.x,
                                    transform->scaling.y,
                                    transform->scaling.z
                                ));

                                // Transform AABB corners to world space
                                glm::vec3 corners[8] = {
                                    glm::vec3(meshData.aabbMin.x, meshData.aabbMin.y, meshData.aabbMin.z),
                                    glm::vec3(meshData.aabbMax.x, meshData.aabbMin.y, meshData.aabbMin.z),
                                    glm::vec3(meshData.aabbMax.x, meshData.aabbMax.y, meshData.aabbMin.z),
                                    glm::vec3(meshData.aabbMin.x, meshData.aabbMax.y, meshData.aabbMin.z),
                                    glm::vec3(meshData.aabbMin.x, meshData.aabbMin.y, meshData.aabbMax.z),
                                    glm::vec3(meshData.aabbMax.x, meshData.aabbMin.y, meshData.aabbMax.z),
                                    glm::vec3(meshData.aabbMax.x, meshData.aabbMax.y, meshData.aabbMax.z),
                                    glm::vec3(meshData.aabbMin.x, meshData.aabbMax.y, meshData.aabbMax.z)
                                };

                                glm::vec3 worldMin(FLT_MAX);
                                glm::vec3 worldMax(-FLT_MAX);

                                for (int i = 0; i < 8; ++i)
                                {
                                    glm::vec4 worldCorner = model * glm::vec4(corners[i], 1.0f);
                                    glm::vec3 corner3D = glm::vec3(worldCorner);

                                    worldMin.x = std::min(worldMin.x, corner3D.x);
                                    worldMin.y = std::min(worldMin.y, corner3D.y);
                                    worldMin.z = std::min(worldMin.z, corner3D.z);

                                    worldMax.x = std::max(worldMax.x, corner3D.x);
                                    worldMax.y = std::max(worldMax.y, corner3D.y);
                                    worldMax.z = std::max(worldMax.z, corner3D.z);
                                }

                                AABB worldAABB(worldMin, worldMax);

                                // Test ray against AABB
                                float tMin, tMax;
                                if (worldAABB.IntersectRay(ray, tMin, tMax))
                                {

                                    float hitDistance = tMin > 0.0f ? tMin : tMax;

                                    if (hitDistance >= 0.0f && hitDistance < closestDistance)
                                    {
                                        closestDistance = hitDistance;
                                        closestObject = go;
                                    }
                                }
                            }
                        }
                    }

                    // Bind texture to the closest game object or inspector object
                    if (closestObject != nullptr)
                    {
                        // New texture data (delete the previous)
                        if (closestObject->texture->texturedata != nullptr)
                        {
                            delete closestObject->texture->texturedata;
                            closestObject->texture->texturedata = nullptr;
                        }

                        if (closestObject->texture->LoadTexture(path)) {
                            std::cout << "Texture assigned successfully to " << closestObject->name << std::endl;
                            LOG("Texture " + path + " assigned to " + closestObject->name);

                            for (GameObject* go : Application::GetInstance().opengl->gameObjects)
                            {
                                if (go->texture->texturedata != nullptr)
                                {
                                    if (go == closestObject) {
                                    }
                                    std::cout << std::endl;
                                }
                                else
                                {
                                    std::cout << "  " << go->name << " -> No texture" << std::endl;
                                }
                            }
                        }
                        else
                        {
                            std::cerr << "Failed to load texture for " << closestObject->name << std::endl;
                        }
                    }
                    else
                    {
                        // If there is no close object
                        std::cout << "No object found under cursor" << std::endl;
                        LOG_WARNING("No GameObject in that position");
                    }
                }
                else
                {
                    LOG_WARNING("Unknown file format");
                }
            }
            break;
        }
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        {
            break;
        }
        }
    }
    return true;
}

std::string Input::GetTexturePathFromFBX(const char* fbxPath, int meshIndex)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(fbxPath,
        aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || meshIndex >= (int)scene->mNumMeshes) {
        return "";
    }

    aiMesh* mesh = scene->mMeshes[meshIndex];
    if (mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < (int)scene->mNumMaterials) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString texPath;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);

            std::string fullPath = texPath.C_Str();

            //get the directory
            std::string fbxDir = fbxPath;
            size_t lastSlash = fbxDir.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                fbxDir = fbxDir.substr(0, lastSlash + 1);
            }

            if (fullPath.find(":") == std::string::npos &&
                fullPath[0] != '/' && fullPath[0] != '\\') {
                fullPath = fbxDir + fullPath;
            }

            return fullPath;
        }
    }

    return "";
}

bool Input::CleanUp()
{
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
    return true;
}

bool Input::GetWindowEvent(EventWindow ev)
{
    return windowEvents[ev];
}