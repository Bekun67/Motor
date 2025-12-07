#include "Input.h"
#include "Window.h"
#include "Application.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "imgui_impl_sdl3.h"
#include <imgui.h>     
#include <ImGuizmo.h>  
#include "EditorPlaySystem.h"

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
            }
        }

        // GameObject selection with F1/F2
        if (keyboard[SDL_SCANCODE_F1] == KEY_DOWN || keyboard[SDL_SCANCODE_F2] == KEY_DOWN)
        {
            int index = -1;
            if (opengl->gameObjects.size() > 0) {
                GameObject* currentSelection = editor->selectedGameObjects.empty() ? nullptr : editor->selectedGameObjects[0];

                for (int i = 0; i < opengl->gameObjects.size(); i++)
                {
                    if (currentSelection == opengl->gameObjects[i]) index = i;
                }

                if (keyboard[SDL_SCANCODE_F2] == KEY_DOWN && index < opengl->gameObjects.size() - 1) {
                    GameObject* nextObject = opengl->gameObjects[index + 1];
                    editor->SelectGameObject(nextObject, false);
                    opengl->selectedGameObject = nextObject;
                    LOG("Selecting next Game Object, " + nextObject->name);
                }
                if (keyboard[SDL_SCANCODE_F1] == KEY_DOWN && index > 0) {
                    GameObject* prevObject = opengl->gameObjects[index - 1];
                    editor->SelectGameObject(prevObject, false);
                    opengl->selectedGameObject = prevObject;
                    LOG("Selecting previous Game Object, " + prevObject->name);
                }
            }
            else {
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

            //sceneViewportPos is located in upper left corner
            int minX = moduleEditor->sceneViewportPos.x;
            int maxX = moduleEditor->sceneViewportPos.x + moduleEditor->sceneViewportSize.x;

            int minY = moduleEditor->sceneViewportPos.y;
            int maxY = moduleEditor->sceneViewportPos.y + moduleEditor->sceneViewportSize.y;

            bool mouseInsideScene = false;
            bool mouseInsideTextureInspector = false;

            if (mouseX > minX && mouseX < maxX &&
                mouseY > minY && mouseY < maxY)
            {
                mouseInsideScene = true;
            }

            if (moduleEditor->showInspector)
            {
                if (moduleEditor->selectedGameObjects.empty())
                    LOG("WARNING: No GameObject selected!");
                //texture drag area;
                else if (mouseX >= moduleEditor->textureDropPos.x && mouseX <= moduleEditor->textureDropPos.x + moduleEditor->textureDropSize.x &&
                    mouseY >= moduleEditor->textureDropPos.y && mouseY <= moduleEditor->textureDropPos.y + moduleEditor->textureDropSize.y)
                {
                    mouseInsideTextureInspector = true;
                }
            }

            if (droppedFile) {
                std::string path(droppedFile);

                //normalize route just in case
                for (size_t i = 0; i < path.size(); ++i) {
                    if (path[i] == '\\') path[i] = '/';
                }

                std::cout << "========================" << std::endl;
                std::cout << "Dropped file: " << path << std::endl;

                //get extension
                std::string extension = "";
                size_t dotPos = path.find_last_of('.');
                if (dotPos != std::string::npos && dotPos < path.length() - 1) {
                    extension = path.substr(dotPos);
                    //convert it to lower case
                    for (size_t i = 0; i < extension.size(); ++i) {
                        extension[i] = (char)tolower(extension[i]);
                    }
                }

                if (extension == ".fbx") {
                    if (!mouseInsideScene)
                    {
                        LOG("WARNING: Drop mesh in scene");
                        break;
                    }
                    //if fbx we load its mesh
                    size_t meshCountBefore = g_Meshes.size();
                    std::cout << "=========MESH===========" << std::endl;

                    if (LoadFile(path.c_str())) {
                        std::cout << "FBX loaded" << std::endl;

                        float desiredSize = 5.0f;
                        float normalizeScale = (g_ModelRadius > 0.001f) ? (desiredSize / g_ModelRadius) : 1.0f;

                        //get mouse pos
                        SDL_GetMouseState(&mouseX, &mouseY);

                        //get camera
                        Camera* camera = &(Application::GetInstance().opengl->camera);
                        int viewport[4];
                        glGetIntegerv(GL_VIEWPORT, viewport);

                        //convert coordinates
                        float x = (2.0f * mouseX) / viewport[2] - 1.0f;
                        float y = 1.0f - (2.0f * mouseY) / viewport[3];

                        //calculate ray
                        glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
                        glm::vec4 rayEye = glm::inverse(camera->GetProjectionMatrix()) * rayClip;
                        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
                        glm::vec3 rayWorld = glm::vec3(glm::inverse(camera->GetViewMatrix()) * rayEye);
                        rayWorld = glm::normalize(rayWorld);

                        //interesct with floor
                        glm::vec3 camPos = camera->GetPosition();
                        float t = -camPos.y / rayWorld.y;
                        glm::vec3 dropPosition = camPos + rayWorld * t;

                        for (size_t i = meshCountBefore; i < g_Meshes.size(); ++i)
                        {
                            //create gameobject with mesh
                            GameObject* go = new GameObject();
                            int index = moduleEditor->CountNames("DroppedMesh_");
                            go->name = "DroppedMesh_" + std::to_string(index);
                            go->meshPath = path;
                            go->meshIndexInFBX = (int)(i - meshCountBefore);

                            //change the translation to match the obtained coordinates
                            go->transform->translation = aiVector3D(dropPosition.x, 0.0f, dropPosition.z);
                            go->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
                            go->transform->scaling = aiVector3D(normalizeScale, normalizeScale, normalizeScale);

                            go->mesh->meshIndex = (int)i;

                            //try to load texture
                            std::string texturePath = GetTexturePathFromFBX(path.c_str(), (int)(i - meshCountBefore));

                            bool textureLoaded = false;
                            if (!texturePath.empty()) {
                                textureLoaded = go->texture->LoadTexture(texturePath);
                            }

                            if (textureLoaded) {
                                std::cout << "Assigned texture from FBX: " << texturePath << std::endl;
                            }
                            else {
                                //if no texture available we use checkerboard
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
                            std::cout << "Created GameObject " << go->name << std::endl;
                            LOG("Created GameObject " + go->name + " with mesh " + path);

                            ModuleEditor* editor = Application::GetInstance().editor.get();
                            if (editor) editor->sceneModified = true;
                        }

                        std::cout << "Total GameObjects in scene: " << Application::GetInstance().opengl->gameObjects.size() << std::endl;
                        std::cout << std::endl;
                    }
                }
                else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
                    //get mouse pos
                    if (!mouseInsideScene && !mouseInsideTextureInspector)
                    {
                        if (!moduleEditor->selectedGameObjects.empty())
                            LOG("WARNING: Drop texture over a GameObject or in the Inspector tab!");
                        break;
                    }
                    std::cout << "========TEXTURE==========" << std::endl;
                    SDL_GetMouseState(&mouseX, &mouseY);

                    //get camera
                    Camera* camera = &(Application::GetInstance().opengl->camera);

                    int viewport[4];
                    glGetIntegerv(GL_VIEWPORT, viewport);

                    //convert coordinates
                    float x = (2.0f * mouseX) / viewport[2] - 1.0f;
                    float y = 1.0f - (2.0f * mouseY) / viewport[3];

                    //create ray
                    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
                    glm::vec4 rayEye = glm::inverse(camera->GetProjectionMatrix()) * rayClip;
                    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
                    glm::vec3 rayWorld = glm::vec3(glm::inverse(camera->GetViewMatrix()) * rayEye);
                    rayWorld = glm::normalize(rayWorld);

                    glm::vec3 camPos = camera->GetPosition();

                    GameObject* closestObject = nullptr;

                    //if we are hovering inside the "Drag new texture here:" panel we change the selectedGameObject's texture
                    if (mouseInsideTextureInspector && !moduleEditor->selectedGameObjects.empty())
                        closestObject = moduleEditor->selectedGameObjects[0];

                    //if we are hovering over the scene we find the closest game object
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

                            // create ray using viewport-relative coordinates
                            Ray ray = mousePicking->CreateRayFromMouse(
                                relativeMouseX,
                                relativeMouseY,
                                camera,
                                (int)viewportWidth,
                                (int)viewportHeight
                            );

                            //test ray against all game objects using AABB
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

                                //transform AABB corners to world space
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

                    //bind texture to the closest game object or inspector object
                    if (closestObject != nullptr)
                    {
                        //new texture data (delete the previous)
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
                        //if there is no close object
                        std::cout << "No object found under cursor" << std::endl;
                        LOG("WARNING: No GameObject in that position");
                    }
                }
                else
                {
                    LOG("WARNING: Unknown file format");
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