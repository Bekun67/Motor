#include "OpenGL.h"
#include "Application.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <iostream>
#include "glm/gtc/quaternion.hpp"
#include <vector>
#include "LoadFBX.h" 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Texture.h"
#include "GameObject.h"

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

void OpenGL::CreateGrid(int size)
{
    std::vector<float> gridVertices;

    // Create lines (X)
    for (int z = -size; z <= size; ++z) {
        
        gridVertices.push_back(-size); // x1
        gridVertices.push_back(0.0f);   // y1
        gridVertices.push_back(z);      // z1

        gridVertices.push_back(size);   // x2
        gridVertices.push_back(0.0f);   // y2
        gridVertices.push_back(z);      // z2
    }

    //Create lines (Z)
    for (int x = -size; x <= size; ++x) {
        
        gridVertices.push_back(x);      // x1
        gridVertices.push_back(0.0f);   // y1
        gridVertices.push_back(-size);  // z1

        gridVertices.push_back(x);      // x2
        gridVertices.push_back(0.0f);   // y2
        gridVertices.push_back(size);   // z2
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
        "vec3 n = normalize(fragNormal);\n"
        "float lambert = max(dot(n, normalize(vec3(0.3, 0.7, 0.5))), 0.0);\n"
        "vec3 texColor = texture(uTexture, fragUV).rgb;\n"
        "FragColor = vec4(texColor * lambert, 1.0);\n"
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

    lastTicks = SDL_GetTicks();

    //load house fbx
    const char* fbxPath = "Assets/Models/BakerHouse.fbx";
    if (!LoadFile(fbxPath)) {
        std::cerr << "Failed to load model: " << fbxPath << std::endl;
        return false;
    }
    else std::cout << "FBX loaded" << std::endl;

    //create gameobject for the house
    GameObject* house = new GameObject();
    house->name = "BakerHouse";
    std::cout << "Created GameObject " << house->name << std::endl;

    //place it in the middle and rotate it (it was facing sideways)
    house->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
    aiQuaternion rotX(aiVector3D(1.0f, 0.0f, 0.0f), glm::radians(90.0f));
    house->transform->rotation = rotX;
    house->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

    //assigned the very first index in our loaded meshes
    if (!g_Meshes.empty()) {
        house->mesh->meshIndex = 0;
    }

    //assign texture
    if (house->texture->LoadTexture("Assets/Textures/Baker_house.png")) {
    }

    //add it to the gameobjects list
    gameObjects.push_back(house);
    std::cout << "Total GameObjects in scene: " << gameObjects.size() << std::endl;

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

    // Draw grid
    DrawGrid();

    // Draw all GameObjects
    for (GameObject* go : gameObjects)
    {
        if (go != nullptr && go->mesh != nullptr)
        {
            go->mesh->Draw(&camera);
        }
    }

    return true;
}

bool OpenGL::CleanUp()
{
    std::cout << "Destroying OpenGL Context" << std::endl;

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