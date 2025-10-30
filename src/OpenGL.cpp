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

OpenGL::OpenGL() : glContext(nullptr), shaderProgram(0)
{
    std::cout << "OpenGL Constructor" << std::endl;
}

OpenGL::~OpenGL()
{
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

    // Check linking
    int success;
    char infoLog[1024];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 1024, NULL, infoLog);
        std::cerr << "ERROR: Shader Program Linking Failed\n" << infoLog << std::endl;
    }

    // we can delete the shader objects after linking
    glDeleteShader(vs);
    glDeleteShader(fs);

    lastTicks = SDL_GetTicks();

    // Load manually FBX
    const char* fbxPath = "Assets/Models/BakerHouse.fbx"; 
    if (!LoadFile(fbxPath)) {
        std::cerr << "Failed to load model: " << fbxPath << std::endl;
    }
    else {
        std::cout << "Loaded FBX meshes: " << g_Meshes.size() << std::endl;
    }

    Texture* modelTexture = new Texture();
    if (!modelTexture->LoadFromFile("Assets/Textures/Baker_house.png")) {
        std::cerr << "Failed to load texture!" << std::endl;
    }
    else {
        std::cout << "Texture loaded successfully!" << std::endl;
    }

	// Save texture info in the first mesh (for simplicity)
    if (!g_Meshes.empty()) {
        TextureData texData;
        texData.id = modelTexture->GetID();
        texData.type = "diffuse";
        texData.path = "Assets/Textures/Baker_house.png";
        g_Meshes[0].textures.push_back(texData);
    }

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
    glUseProgram(shaderProgram);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model_matrix");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.01f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1, 0, 0));

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = camera.GetProjectionMatrix();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    for (const MeshData& md : g_Meshes) {
        if (md.VAO == 0 || md.numIndices == 0) continue;

        // Activate existing texture
        if (!md.textures.empty()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, md.textures[0].id);

            GLint texLoc = glGetUniformLocation(shaderProgram, "uTexture");
            glUniform1i(texLoc, 0); 
        }

        glBindVertexArray(md.VAO);
        glDrawElements(GL_TRIANGLES, md.numIndices, GL_UNSIGNED_INT, 0);
    }
    // glBindVertexArray(VAO); glDrawArrays(GL_TRIANGLES, 0, 3);

    return true;
}

bool OpenGL::CleanUp()
{
    std::cout << "Destroying OpenGL Context" << std::endl;

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
        md = MeshData(); // reset
    }
    g_Meshes.clear();

    // Delete shader program
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }

    // Destroy context
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