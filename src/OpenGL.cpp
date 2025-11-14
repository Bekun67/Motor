#include "ModuleEditor.h"
#include "OpenGL.h"
#include "Application.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <iostream>
#include "glm/gtc/quaternion.hpp"
#include <vector>
#include "LoadFBX.h" 
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Texture.h"
#include "GameObject.h"
#include "MeshImporter.h"
#include "TextureImporter.h"
#include <chrono>


OpenGL::OpenGL() : glContext(nullptr), shaderProgram(0)
{
    std::cout << "OpenGL Constructor" << std::endl;
}

OpenGL::~OpenGL()
{
}

OpenGL& OpenGL::GetInstance() {
    static OpenGL instance; 
    return instance;
}

static GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        std::cerr << "Shader compile error: " << infoLog << std::endl;
    }
    return shader;
}

// Create normals shaders
static GLuint CreateNormalShader() {
    const char* vertexShaderSource = "#version 330 core\n"
        "layout(location = 0) in vec3 position;\n"
        "uniform mat4 model_matrix;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = projection * view * model_matrix * vec4(position, 1.0);\n"
        "}\n";

    const char* fragmentShaderSource = "#version 330 core\n"
        "in vec3 fragNormal;\n"
        "in vec2 fragUV;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D uTexture;\n"
        "void main() {\n"
        "    vec3 n = normalize(fragNormal);\n"
        "    float lambert = max(dot(n, normalize(vec3(0.3, 0.7, 0.5))), 0.0);\n"
        "    vec4 texColor = texture(uTexture, fragUV);\n"
        "    \n"
        "    if (texColor.a < 0.1) discard;\n"
        "    \n"
        "    FragColor = vec4(texColor.rgb * lambert, texColor.a);\n"
        "}\n";

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int success;
    char infoLog[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, NULL, infoLog);
        std::cerr << "ERROR: Normal Shader Program Linking Failed\n" << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    std::cout << "Normal shader created successfully" << std::endl;
    return program;

}


void OpenGL::CreateGrid(int size)
{
    std::vector<float> gridVertices;

    // Create lines (X axis)
    for (int z = -size; z <= size; ++z) {
        
        gridVertices.push_back(-size); 
        gridVertices.push_back(0.0f);   
        gridVertices.push_back(z);      

        gridVertices.push_back(size);   
        gridVertices.push_back(0.0f);   
        gridVertices.push_back(z);      
    }

    //Create lines (Z axis)
    for (int x = -size; x <= size; ++x) {
        
        gridVertices.push_back(x);    
        gridVertices.push_back(0.0f);   
        gridVertices.push_back(-size); 

        gridVertices.push_back(x);     
        gridVertices.push_back(0.0f);  
        gridVertices.push_back(size);   
    }

    gridLineCount = gridVertices.size() / 3;

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);

    // Position for grid
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    std::cout << "Grid created with " << (size * 2 + 1) * 2 << " lines" << std::endl;
}

void OpenGL::DrawGrid()
{
    if (!showGrid || gridVAO == 0) return;

    glUseProgram(shaderProgram);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model_matrix");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

    //identiti matrix for grid
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = camera.GetProjectionMatrix();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, gridLineCount);
    glBindVertexArray(0);
}


bool OpenGL::Start()
{
    std::cout << "Init OpenGL Context & GLAD" << std::endl;

    SDL_Window* window = Application::GetInstance().window->GetWindow();
    glContext = SDL_GL_CreateContext(window);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }


    // Start Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;


    ImGui::StyleColorsClassic();

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 330");
    LOG("ImGui initialized successfully");

    // Do a depth test
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);


    // Shader using position normal texcoord and matrix
    const char* vertexShaderSource = "#version 330 core\n"
        "layout(location = 0) in vec3 position;\n"
        "layout(location = 1) in vec3 normal;\n"
        "layout(location = 2) in vec2 texcoord;\n"
        "out vec3 fragNormal;\n"
        "out vec2 fragUV;\n"
        "uniform mat4 model_matrix;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    fragNormal = mat3(transpose(inverse(model_matrix))) * normal;\n"
        "    fragUV = texcoord;\n"
        "    gl_Position = projection * view * model_matrix * vec4(position, 1.0);\n"
        "}\n";

    const char* fragmentShaderSource = "#version 330 core\n"
        "in vec3 fragNormal;\n"
        "in vec2 fragUV;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D uTexture;\n"
        "void main() {\n"
        "    vec3 n = normalize(fragNormal);\n"
        "    float lambert = max(dot(n, normalize(vec3(0.3, 0.7, 0.5))), 0.0);\n"
        "    vec4 texColor = texture(uTexture, fragUV);\n"
        "    FragColor = vec4(texColor.rgb * lambert, texColor.a);\n"
        "}\n";

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[1024];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 1024, NULL, infoLog);
        std::cerr << "ERROR: Shader Program Linking Failed\n" << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    // Create shaders for normals
    normalShaderProgram = CreateNormalShader();

    lastTicks = SDL_GetTicks();

    const char* fbxPath = "Assets/Models/BakerHouse.fbx";
    const char* texturePath = "Assets/Textures/Baker_house.png";

    // Check if we need to reimport MESH
    std::string customMeshPath = MeshImporter::GetCustomMeshPath(fbxPath, 0);
    bool needsMeshReimport = FileSystemManager::NeedsReimport(fbxPath, customMeshPath);

    // Check if we need to reimport TEXTURE
    std::string customTexturePath = TextureImporter::GetCustomTexturePath(texturePath);
    bool needsTextureReimport = FileSystemManager::NeedsReimport(texturePath, customTexturePath);

    // MESH IMPORT
    if (needsMeshReimport) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "BAKER HOUSE - First time or FBX modified" << std::endl;
        std::cout << "========================================" << std::endl;

        // Import from FBX
        std::cout << "[BakerHouse] Importing from FBX..." << std::endl;
        auto importStart = std::chrono::high_resolution_clock::now();
        std::vector<CustomMesh> meshes = MeshImporter::ImportFBX(fbxPath);
        auto importEnd = std::chrono::high_resolution_clock::now();
        auto importDuration = std::chrono::duration_cast<std::chrono::milliseconds>(importEnd - importStart);

        if (meshes.empty()) {
            std::cerr << "[BakerHouse] Failed to import FBX" << std::endl;
        }
        else {
            std::cout << "[BakerHouse] Import completed in " << importDuration.count() << " ms" << std::endl;

            // Save to custom format
            std::cout << "[BakerHouse] Saving to custom format..." << std::endl;
            for (size_t i = 0; i < meshes.size(); ++i) {
                std::string savePath = MeshImporter::GetCustomMeshPath(fbxPath, i);
                if (!MeshImporter::SaveMesh(meshes[i], savePath)) {
                    std::cerr << "[BakerHouse] Failed to save mesh " << i << std::endl;
                }
            }
        }
    }

    // TEXTURE IMPORT
    if (needsTextureReimport) {
        std::cout << "\n[BakerHouse] Texture needs reimport" << std::endl;

        // Import texture
        auto texImportStart = std::chrono::high_resolution_clock::now();
        CustomTexture customTex = TextureImporter::ImportTexture(texturePath);
        auto texImportEnd = std::chrono::high_resolution_clock::now();
        auto texImportDuration = std::chrono::duration_cast<std::chrono::milliseconds>(texImportEnd - texImportStart);

        if (customTex.width > 0 && customTex.height > 0) {
            std::cout << "[BakerHouse] Texture imported in " << texImportDuration.count() << " ms" << std::endl;

            // Save to custom format
            if (TextureImporter::SaveTexture(customTex, customTexturePath)) {
                std::cout << "[BakerHouse] Texture saved to custom format" << std::endl;
            }
        }
        else {
            std::cerr << "[BakerHouse] Failed to import texture" << std::endl;
        }
    }

    // NOW LOAD MESH FROM CUSTOM FORMAT
    std::cout << "\n[BakerHouse] Loading mesh from custom format..." << std::endl;
    if (!LoadFileCustomFormat(fbxPath)) {
        std::cerr << "[BakerHouse] Failed to load mesh from custom format" << std::endl;
    }

    // CREATE GAME OBJECT
    if (!g_Meshes.empty()) {
        GameObject* house = new GameObject();
        house->meshPath = fbxPath;
        house->name = "BakerHouse";
        std::cout << "Created GameObject " << house->name << std::endl;

        house->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
        aiQuaternion rotX(aiVector3D(1.0f, 0.0f, 0.0f), glm::radians(90.0f));
        house->transform->rotation = rotX;
        house->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

        house->mesh->meshIndex = 0;

        // LOAD TEXTURE FROM CUSTOM FORMAT
        CustomTexture loadedTexture;
        if (TextureImporter::LoadTexture(loadedTexture, customTexturePath)) {
            std::cout << "[BakerHouse] Loading texture from custom format..." << std::endl;

            // Create OpenGL texture from custom data
            GLuint textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            // Upload texture data
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                loadedTexture.width, loadedTexture.height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, loadedTexture.data.data());

            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glBindTexture(GL_TEXTURE_2D, 0);

            // Assign to GameObject
            house->texture->hasTexture = true;
            if (house->texture->texturedata == nullptr) {
                house->texture->texturedata = new TextureData();
            }
            house->texture->texturedata->id = textureID;
            house->texture->texturedata->type = "diffuse";
            house->texture->texturedata->path = customTexturePath;
            house->texture->texturePath = customTexturePath;

            std::cout << "[BakerHouse] Texture loaded from custom format (ID: " << textureID << ")" << std::endl;
        }
        else {
            std::cerr << "[BakerHouse] Failed to load texture from custom format, using fallback" << std::endl;
            // Fallback to loading original texture
            if (!house->texture->LoadTexture(texturePath)) {
                std::cerr << "[BakerHouse] Failed to load original texture" << std::endl;
            }
        }

        gameObjects.push_back(house);
        std::cout << "[BakerHouse] Added to scene" << std::endl;
    }
    else {
        std::cerr << "[BakerHouse] No meshes loaded!" << std::endl;
    }

    std::cout << "\n========================================\n" << std::endl;
    //load cannon FBX
    const char* cannonPath = "Assets/Models/Cannon.fbx";
    size_t meshCountBefore = g_Meshes.size();

    if (!LoadFile(cannonPath)) {
        std::cerr << "Failed to load model: " << cannonPath << std::endl;
    }
    else {
        std::cout << "Cannon FBX loaded" << std::endl;

        //create the one that has texture
        GameObject* cannon1 = new GameObject();
        cannon1->meshPath = cannonPath;
        cannon1->name = "Cannon_Left";
        std::cout << "Created GameObject " << cannon1->name << std::endl;

        cannon1->transform->translation = aiVector3D(-5.0f, 0.0f, 0.0f);
        cannon1->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
        cannon1->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

        if (meshCountBefore < g_Meshes.size()) {
            cannon1->mesh->meshIndex = (int)meshCountBefore;
        }

        //assing lenna texture
        if (cannon1->texture->LoadTexture("Assets/Textures/lenna.png")) {
            std::cout << "Texture assigned to " << cannon1->name << std::endl;
        }
        else {
            std::cout << "Failed to load texture for " << cannon1->name << std::endl;
        }

        gameObjects.push_back(cannon1);

        //create the one that has no texture
        GameObject* cannon2 = new GameObject();
        cannon2->meshPath = cannonPath;
        cannon2->name = "Cannon_Right";
        std::cout << "Created GameObject " << cannon2->name << std::endl;

        cannon2->transform->translation = aiVector3D(5.0f, 0.0f, 0.0f);
        cannon2->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
        cannon2->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

        if (meshCountBefore < g_Meshes.size()) {
            cannon2->mesh->meshIndex = (int)meshCountBefore;
        }

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

        cannon2->texture->hasTexture = true;
        if (cannon2->texture->texturedata == nullptr) {
            cannon2->texture->texturedata = new TextureData();
        }
        cannon2->texture->texturedata->id = checkID;
        cannon2->texture->texturedata->type = "checkerboard";
        cannon2->texture->texturedata->path = "checkerboard";

        std::cout << cannon2->name << " created without texture (will use checkerboard)" << std::endl;
        gameObjects.push_back(cannon2);

        std::cout << "Total GameObjects in scene: " << gameObjects.size() << std::endl;
    }
    std::cout << std::endl;

    CreateGrid(50);

    std::cout << "OpenGL initialized successfully" << std::endl;
    return true;
}

bool OpenGL::Update()
{
    // Calculate deltatime
    uint64_t currentTicks = SDL_GetTicks();
    float deltaTime = (currentTicks - lastTicks) / 1000.0f;
    lastTicks = currentTicks;

    // Use camera input handling
    camera.HandleInput(deltaTime);

    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ModuleEditor* editor = Application::GetInstance().editor.get();

    // Configurar el viewport para que OpenGL solo renderice en el área de la escena
    if (editor)
    {
        // Convertir coordenadas de ImGui a coordenadas de OpenGL (Y invertida)
        Window* window = Application::GetInstance().window.get();
        int windowWidth, windowHeight;
        window->GetWindowSize(windowWidth, windowHeight);

        // ImGui usa coordenadas desde arriba, OpenGL desde abajo
        int viewportX = (int)editor->sceneViewportPos.x;
        int viewportY = windowHeight - (int)(editor->sceneViewportPos.y + editor->sceneViewportSize.y);
        int viewportWidth = (int)editor->sceneViewportSize.x;
        int viewportHeight = (int)editor->sceneViewportSize.y;

        // Establecer el viewport de OpenGL
        glViewport(viewportX, viewportY, viewportWidth, viewportHeight);

        // Actualizar el aspect ratio de la cámara basado en el viewport
        if (viewportHeight > 0)
        {
            camera.aspect = (float)viewportWidth / (float)viewportHeight;
        }
    }
    else
    {
        // Fallback: usar toda la ventana
        Window* window = Application::GetInstance().window.get();
        int windowWidth, windowHeight;
        window->GetWindowSize(windowWidth, windowHeight);
        glViewport(0, 0, windowWidth, windowHeight);
    }

    // Clear solo el área del viewport
    glClearColor(0.15f, 0.15f, 0.17f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // Draw grid
    DrawGrid();

    std::vector<GameObject*> opaqueObjects;
    std::vector<GameObject*> transparentObjects;

    glm::vec3 cameraPos = camera.GetPosition();

    //we split game objects depending on their transaparency
    for (GameObject* go : gameObjects)
    {
        if (go != nullptr && go->mesh != nullptr && go->mesh->meshIndex >= 0)
        {
            //calculate distance
            float dx = cameraPos.x - go->transform->translation.x;
            float dy = cameraPos.y - go->transform->translation.y;
            float dz = cameraPos.z - go->transform->translation.z;
            go->distanceToCamera = sqrt(dx * dx + dy * dy + dz * dz);

            //filter transparent and opaque
            bool isTransparent = false;
            if (go->texture != nullptr && go->texture->hasTexture)
            {
                isTransparent = go->texture->hasTransparency;
            }

            if (isTransparent)
            {
                transparentObjects.push_back(go);
            }
            else
            {
                opaqueObjects.push_back(go);
            }
        }
    }

    //order transparent obj based on distance (method given by #include <algorithm>)
    std::sort(transparentObjects.begin(), transparentObjects.end(),
        [](GameObject* a, GameObject* b) 
        {
            return a->distanceToCamera > b->distanceToCamera;
        });

    //first drawing opaque obj
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    for (GameObject* go : opaqueObjects)
    {
        go->mesh->Draw(&camera);
    }

    //then transparent obj
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (GameObject* go : transparentObjects)
    {
        go->mesh->Draw(&camera);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    return true;
}

void OpenGL::MySaveFunction()
{

}

bool OpenGL::CleanUp()
{
    std::cout << "Destroying OpenGL Context" << std::endl;

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    LOG("ImGui cleaned up");

    // Delete all GameObjects
    for (GameObject* go : gameObjects)
    {
        delete go;
    }
    gameObjects.clear();

    // Delete loaded resources by LoadFBX
    for (MeshData& md : g_Meshes) {
        for (TextureData& tex : md.textures) {
            if (tex.id != 0)
                glDeleteTextures(1, &tex.id);
        }
        md.textures.clear();

        if (md.EBO) glDeleteBuffers(1, &md.EBO);
        if (md.VBO) glDeleteBuffers(1, &md.VBO);
        if (md.VAO) glDeleteVertexArrays(1, &md.VAO);
        md = MeshData();
    }
    g_Meshes.clear();

    // Delete grid buffers
    if (gridVBO) glDeleteBuffers(1, &gridVBO);
    if (gridVAO) glDeleteVertexArrays(1, &gridVAO);

    // Delete shader program
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }

    if (normalShaderProgram) {
        glDeleteProgram(normalShaderProgram);
        normalShaderProgram = 0;
    }

    if (glContext != nullptr)
    {
        SDL_GL_DestroyContext(glContext);
        glContext = nullptr;
    }


    return true;
}

bool OpenGL::Draw()
{
    return true;
}