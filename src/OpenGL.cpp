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
        "out vec4 FragColor;\n"
        "uniform vec3 lineColor;\n"
        "void main() {\n"
        "    FragColor = vec4(lineColor, 1.0);\n"
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

// Create outline shaders detecting the edge
static GLuint CreateEdgeDetectionShader() {
    const char* vertexShaderSource = "#version 330 core\n"
        "layout(location = 0) in vec2 aPos;\n"
        "layout(location = 1) in vec2 aTexCoord;\n"
        "out vec2 TexCoord;\n"
        "void main() {\n"
        "    TexCoord = aTexCoord;\n"
        "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
        "}\n";

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D maskTexture;\n"
        "uniform vec3 outlineColor;\n"
        "uniform float outlineThickness;\n"
        "uniform vec2 texelSize;\n"
        "void main() {\n"
        "    float mask = texture(maskTexture, TexCoord).r;\n"
        "    \n"
        "    if (mask > 0.5) {\n"
        "        // Check neighbors\n"
        "        float edge = 0.0;\n"
        "        for (float x = -outlineThickness; x <= outlineThickness; x += 1.0) {\n"
        "            for (float y = -outlineThickness; y <= outlineThickness; y += 1.0) {\n"
        "                vec2 offset = vec2(x, y) * texelSize;\n"
        "                float neighborMask = texture(maskTexture, TexCoord + offset).r;\n"
        "                if (neighborMask < 0.5) {\n"
        "                    edge = 1.0;\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "        \n"
        "        if (edge > 0.5) {\n"
        "            FragColor = vec4(outlineColor, 1.0);\n"
        "        } else {\n"
        "            discard;\n"
        "        }\n"
        "    } else {\n"
        "        discard;\n"
        "    }\n"
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
        std::cerr << "ERROR: Edge Detection Shader Linking Failed\n" << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    std::cout << "Edge detection shader created successfully" << std::endl;
    return program;
}

static GLuint CreateMaskShader() {
    const char* vertexShaderSource = "#version 330 core\n"
        "layout(location = 0) in vec3 position;\n"
        "uniform mat4 model_matrix;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = projection * view * model_matrix * vec4(position, 1.0);\n"
        "}\n";

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
        "}\n";

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

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

    ModuleEditor::SetupImGuiStyle();

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
    // Create shader for outline detecting edges
    edgeDetectionShader = CreateEdgeDetectionShader();
    maskShader = CreateMaskShader();

    lastTicks = SDL_GetTicks();

    const char* fbxPath = "Assets/Models/BakerHouse.fbx";
    const char* texturePath = "Assets/Textures/Baker_house.png";

    // Check if we need to reimport MESH
    std::string customMeshPath = MeshImporter::GetCustomMeshPath(fbxPath, 0);
    bool needsMeshReimport = FileSystemManager::NeedsReimport(fbxPath, customMeshPath);

    // Check if we need to reimport TEXTURE
    std::string customTexturePath = TextureImporter::GetCustomTexturePath(texturePath);
    bool needsTextureReimport = FileSystemManager::NeedsReimport(texturePath, customTexturePath);

    // Mesh import
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

    // texture import
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

    // Load mesh from custom format
    std::cout << "\n[BakerHouse] Loading mesh from custom format..." << std::endl;
    if (!LoadFileCustomFormat(fbxPath)) {
        std::cerr << "[BakerHouse] Failed to load mesh from custom format" << std::endl;
    }

    // Create game object
    if (!g_Meshes.empty()) {
        GameObject* house = new GameObject();
        house->meshPath = fbxPath;
        house->name = "BakerHouse";
        house->meshIndexInFBX = 0;
        std::cout << "Created GameObject " << house->name << std::endl;

        house->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
        aiQuaternion rotX(aiVector3D(1.0f, 0.0f, 0.0f), glm::radians(90.0f));
        house->transform->rotation = rotX;
        house->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

        house->mesh->meshIndex = 0;

        // Load texture from custom format
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
        cannon1->meshIndexInFBX = 0;
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
        cannon2->meshIndexInFBX = 0;
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

    camera.Start();
    CreateGrid(50);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    //color texture for zbuffer
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    //depth texture
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //vertex shader for z-buffer
    const char* depthQuadVS = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            TexCoord = aTexCoord;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
        )";

    //fragment shader for z-buffer
    const char* depthQuadFS = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D depthTex;
        void main() {
            float depth = texture(depthTex, TexCoord).r;
            FragColor = vec4(vec3(1.0 - depth), 1.0);
        }
        )";

    depthDebugShader = glCreateProgram();
    GLuint quadVS = CompileShader(GL_VERTEX_SHADER, depthQuadVS);
    GLuint quadFS = CompileShader(GL_FRAGMENT_SHADER, depthQuadFS);
    glAttachShader(depthDebugShader, quadVS);
    glAttachShader(depthDebugShader, quadFS);
    glLinkProgram(depthDebugShader);
    glDeleteShader(quadVS);
    glDeleteShader(quadFS);

    float quadVertices[] = 
    {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };
    glGenVertexArrays(1, &fullscreenQuadVAO);
    glGenBuffers(1, &fullscreenQuadVBO);
    glBindVertexArray(fullscreenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fullscreenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Create selection framebuffer for outline
    glGenFramebuffers(1, &selectionFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, selectionFBO);

    glGenTextures(1, &selectionTexture);
    glBindTexture(GL_TEXTURE_2D, selectionTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1024, 1024, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, selectionTexture, 0);

    // Attach depth buffer for proper depth testing
    GLuint selectionDepthBuffer;
    glGenRenderbuffers(1, &selectionDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, selectionDepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1024, 1024);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, selectionDepthBuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR: Selection Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    selectionTextureWidth = 1024;
    selectionTextureHeight = 1024;


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

    // Check if window was resized
    Window* window = Application::GetInstance().window.get();
    if (window->WasResized())
    {
        int windowWidth, windowHeight;
        window->GetWindowSize(windowWidth, windowHeight);
        LOG("OpenGL: Adapting to new window size: " + std::to_string(windowWidth) + "x" + std::to_string(windowHeight));

        // Update editor layout
        ModuleEditor* editor = Application::GetInstance().editor.get();
        if (editor)
        {
            editor->UpdateLayout(windowWidth, windowHeight);
        }
    }

    glClearColor(0.15f, 0.15f, 0.17f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    ModuleEditor* editor = Application::GetInstance().editor.get();

    int viewportWidth = 800;
    int viewportHeight = 600;
    int viewportX = 0;
    int viewportY = 0;

    if (editor)
    {
        Window* window = Application::GetInstance().window.get();
        int windowWidth, windowHeight;
        window->GetWindowSize(windowWidth, windowHeight);

        viewportX = (int)editor->sceneViewportPos.x;
        viewportY = windowHeight - (int)(editor->sceneViewportPos.y + editor->sceneViewportSize.y);
        viewportWidth = (int)editor->sceneViewportSize.x;
        viewportHeight = (int)editor->sceneViewportSize.y;

        if (viewportHeight > 0)
        {
            camera.aspect = (float)viewportWidth / (float)viewportHeight;
        }
    }
    else
    {
        Window* window = Application::GetInstance().window.get();
        int windowWidth, windowHeight;
        window->GetWindowSize(windowWidth, windowHeight);
        viewportWidth = windowWidth;
        viewportHeight = windowHeight;
    }

    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    glClearColor(0.15f, 0.15f, 0.17f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //resize texture if we change the viewport
    if (debugZBuffer)
    {
        if (depthTextureWidth != viewportWidth || depthTextureHeight != viewportHeight)
        {
            depthTextureWidth = viewportWidth;
            depthTextureHeight = viewportHeight;

            glBindTexture(GL_TEXTURE_2D, depthTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, viewportWidth, viewportHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

            glBindTexture(GL_TEXTURE_2D, colorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, viewportWidth, viewportHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        }
    }

    ComponentCamera* editorCam = camera.GetCameraComponent();
    const Frustum* frustum = nullptr;

    if (camera.frustumCullingEnabled && editorCam)
    {
        editorCam->UpdateFrustum();
        frustum = &editorCam->GetFrustum();
    }

    //reset culling stats for showing
    culledCount = 0;
    renderedCount = 0;

    std::vector<GameObject*> opaqueObjects;
    std::vector<GameObject*> transparentObjects;

    glm::vec3 cameraPos = camera.GetPosition();

    //split objects by transparency and apply frustum culling
    for (GameObject* go : gameObjects)
    {
        // Skip empty GameObjects (no mesh)
        if (go == nullptr || go->mesh == nullptr || go->mesh->meshIndex < 0 || go->IsEmpty())
        {
            continue;
        }

        if (go != nullptr && go->mesh != nullptr && go->mesh->meshIndex >= 0)
        {
            //calculate distance
            float dx = cameraPos.x - go->transform->translation.x;
            float dy = cameraPos.y - go->transform->translation.y;
            float dz = cameraPos.z - go->transform->translation.z;
            go->distanceToCamera = sqrt(dx * dx + dy * dy + dz * dz);
        }
        
        //calculate distance to camera
        float dx = cameraPos.x - go->transform->translation.x;
        float dy = cameraPos.y - go->transform->translation.y;
        float dz = cameraPos.z - go->transform->translation.z;
        go->distanceToCamera = sqrt(dx * dx + dy * dy + dz * dz);

        //assume visible by default
        go->isVisibleInFrustum = true;

        //test frustum
        if (frustum != nullptr && go->mesh->meshIndex < (int)g_Meshes.size())
        {
            MeshData& meshData = g_Meshes[go->mesh->meshIndex];

            //get aabb
            glm::vec3 localMin = meshData.aabbMin;
            glm::vec3 localMax = meshData.aabbMax;

            glm::mat4 model = glm::mat4(1.0f);

            //translation
            model = glm::translate(model, glm::vec3(
                go->transform->translation.x,
                go->transform->translation.y,
                go->transform->translation.z
            ));

            //rotation
            glm::quat rotation(
                go->transform->rotation.w,
                go->transform->rotation.x,
                go->transform->rotation.y,
                go->transform->rotation.z
            );
            model *= glm::mat4_cast(rotation);

            //scale
            model = glm::scale(model, glm::vec3(
                go->transform->scaling.x,
                go->transform->scaling.y,
                go->transform->scaling.z
            ));

            //transform 8 aabb corners
            glm::vec3 corners[8] = {
                glm::vec3(localMin.x, localMin.y, localMin.z),
                glm::vec3(localMax.x, localMin.y, localMin.z),
                glm::vec3(localMax.x, localMax.y, localMin.z),
                glm::vec3(localMin.x, localMax.y, localMin.z),
                glm::vec3(localMin.x, localMin.y, localMax.z),
                glm::vec3(localMax.x, localMin.y, localMax.z),
                glm::vec3(localMax.x, localMax.y, localMax.z),
                glm::vec3(localMin.x, localMax.y, localMax.z)
            };

            glm::vec3 worldMin = glm::vec3(FLT_MAX);
            glm::vec3 worldMax = glm::vec3(-FLT_MAX);

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

            //test aabb against frustum
            FrustumIntersection result = frustum->ContainsAABB(worldMin, worldMax);

            //if the object is OUT we skip it
            if (result == FrustumIntersection::OUT)
            {
                go->isVisibleInFrustum = false;
                go->culledLastFrame = true;
                culledCount++;
                continue;
            }
            else
            {
                go->culledLastFrame = false;
                renderedCount++;
            }
        }
        else
        {
            //if the object is IN or INTERESECT we count it
            renderedCount++;
        }

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

    if (!debugZBuffer)
    {
        ModuleEditor* editor = Application::GetInstance().editor.get();

        // Resize selection texture if needed
        if (editor && (selectionTextureWidth != viewportWidth || selectionTextureHeight != viewportHeight))
        {
            selectionTextureWidth = viewportWidth;
            selectionTextureHeight = viewportHeight;

            glBindTexture(GL_TEXTURE_2D, selectionTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, viewportWidth, viewportHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
        }

        // Render selected objects to selection mask
        if (editor && !editor->selectedGameObjects.empty())
        {
            glBindFramebuffer(GL_FRAMEBUFFER, selectionFBO);
            glViewport(0, 0, viewportWidth, viewportHeight);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(maskShader);

            for (GameObject* selectedGO : editor->selectedGameObjects)
            {
                if (selectedGO && selectedGO->mesh &&
                    selectedGO->mesh->meshIndex >= 0 &&
                    !selectedGO->IsEmpty())
                {
                    ComponentTransform* transform = selectedGO->transform;
                    if (!transform) continue;

                    MeshData& meshdata = g_Meshes[selectedGO->mesh->meshIndex];

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

                    glm::mat4 view = camera.GetViewMatrix();
                    glm::mat4 projection = camera.GetProjectionMatrix();

                    glUniformMatrix4fv(glGetUniformLocation(maskShader, "model_matrix"), 1, GL_FALSE, glm::value_ptr(model));
                    glUniformMatrix4fv(glGetUniformLocation(maskShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
                    glUniformMatrix4fv(glGetUniformLocation(maskShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

                    glBindVertexArray(meshdata.VAO);
                    glDrawElements(GL_TRIANGLES, meshdata.numIndices, GL_UNSIGNED_INT, 0);
                    glBindVertexArray(0);
                }
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // Render scene normally
        glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
        glClearColor(0.15f, 0.15f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        DrawGrid();

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        for (GameObject* go : opaqueObjects)
        {
            go->mesh->Draw(&camera);
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        for (GameObject* go : transparentObjects)
        {
            go->mesh->Draw(&camera);
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // Draw outline using edge detection
        if (editor && !editor->selectedGameObjects.empty())
        {
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(edgeDetectionShader);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, selectionTexture);
            glUniform1i(glGetUniformLocation(edgeDetectionShader, "maskTexture"), 0);

            glm::vec3 outlineColor(0.3f, 0.5f, 1.0f);
            glUniform3f(glGetUniformLocation(edgeDetectionShader, "outlineColor"),
                outlineColor.r, outlineColor.g, outlineColor.b);
            glUniform1f(glGetUniformLocation(edgeDetectionShader, "outlineThickness"), 2.0f);
            glUniform2f(glGetUniformLocation(edgeDetectionShader, "texelSize"),
                1.0f / viewportWidth, 1.0f / viewportHeight);

            glBindVertexArray(fullscreenQuadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }
    }
    else
    {
        // Z-buffer debug...
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, viewportWidth, viewportHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        DrawGrid();

        for (GameObject* go : opaqueObjects)
        {
            if (go && go->mesh) go->mesh->Draw(&camera);
        }
        for (GameObject* go : transparentObjects)
        {
            if (go && go->mesh) go->mesh->Draw(&camera);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(depthDebugShader);
        glBindVertexArray(fullscreenQuadVAO);
        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glUniform1i(glGetUniformLocation(depthDebugShader, "depthTex"), 0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glEnable(GL_DEPTH_TEST);
    }

    if (editorCam && editorCam->debugRaycastEnabled)
    {
        for (GameObject* go : opaqueObjects)
        {
            if (go && go->mesh) go->mesh->DrawDebugRay(&camera);
        }
        for (GameObject* go : transparentObjects)
        {
            if (go && go->mesh) go->mesh->DrawDebugRay(&camera);
        }
    }

    return true;
}

bool OpenGL::CleanUp()
{
    std::cout << "Destroying OpenGL Context" << std::endl;

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    LOG("ImGui cleaned up");

    for (GameObject* go : gameObjects)
    {
        if (go != nullptr)
        {
            go->parent = nullptr;
            go->children.clear();
        }
    }

    for (GameObject* go : gameObjects)
    {
        if (go != nullptr)
        {
            go->m_IsBeingDestroyed = true;
            delete go;
        }
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

    if (edgeDetectionShader) {
        glDeleteProgram(edgeDetectionShader);
        edgeDetectionShader = 0;
    }

    if (selectionFBO) {
        glDeleteFramebuffers(1, &selectionFBO);
        selectionFBO = 0;
    }

    if (maskShader) {
        glDeleteProgram(maskShader);
        maskShader = 0;
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