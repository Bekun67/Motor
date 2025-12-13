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

    editor = Application::GetInstance().editor.get();

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
    Window* windowSize = Application::GetInstance().window.get();
    int windowWidth, windowHeight;
    windowSize->GetWindowSize(windowWidth, windowHeight);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, windowWidth, windowHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, selectionTexture, 0);

    // dynamic size for renderbuffer
    GLuint selectionDepthBuffer;
    glGenRenderbuffers(1, &selectionDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, selectionDepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 3840, 2160);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, selectionDepthBuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR: Selection Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    selectionTextureWidth = windowWidth;
    selectionTextureHeight = windowHeight;

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
    if (!editor->editing) camera.HandleInput(deltaTime);

    if (useQuadtree)
    {
        bool needsRebuild = false;

        for (GameObject* go : gameObjects)
        {
            if (go != nullptr && go->transform != nullptr)
            {
                // Check if transform changed
                if (go->transform->HasChanged())
                {
                    if (go->isStatic)
                    {
                        needsRebuild = true;
                    }
                }
            }
        }

        if (needsRebuild)
        {
            LOG("Static object moved - Rebuilding Octree");
            RebuildQuadtree();
        }
    }

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

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    ModuleEditor* editor = Application::GetInstance().editor.get();

    int viewportWidth = 800;
    int viewportHeight = 600;
    int viewportX = 0;
    int viewportY = 0;

    if (editor)
    {
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
        int windowWidth, windowHeight;
        window->GetWindowSize(windowWidth, windowHeight);
        viewportWidth = windowWidth;
        viewportHeight = windowHeight;
    }

    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
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
    quadtreeTestsCount = 0;
    int quadtreeCulledCount = 0;

    glm::vec3 cameraPos = camera.GetPosition();

    std::vector<GameObject*> staticObjects;
    std::vector<GameObject*> dynamicObjects;

    //split dynamic and static go
    for (GameObject* go : gameObjects)
    {
        if (go == nullptr || go->mesh == nullptr || go->mesh->meshIndex < 0 || go->IsEmpty())
            continue;

        //calculate distance to camera
        float dx = cameraPos.x - go->transform->translation.x;
        float dy = cameraPos.y - go->transform->translation.y;
        float dz = cameraPos.z - go->transform->translation.z;
        go->distanceToCamera = sqrt(dx * dx + dy * dy + dz * dz);

        if (go->isStatic)
        {
            staticObjects.push_back(go);
        }
        else
        {
            dynamicObjects.push_back(go);
        }
    }

    //process static obj
    std::vector<GameObject*> visibleStatic;

    if (useQuadtree && !staticObjects.empty())
    {
        if (frustum != nullptr)
        {
            //quadtree returns candidates
            std::vector<GameObject*> candidateObjects;
            quadtree.CollectIntersections(candidateObjects, *frustum);

            quadtreeCulledCount = staticObjects.size() - candidateObjects.size();

            //frustum over all candidates
            quadtreeTestsCount = 0;
            for (GameObject* go : candidateObjects)
            {
                if (go == nullptr || go->mesh == nullptr || go->mesh->meshIndex < 0) continue;
                if (go->mesh->meshIndex >= (int)g_Meshes.size()) continue;

                quadtreeTestsCount++;

                WorldAABB worldAABB = go->mesh->GetWorldAABB();

                if (frustum->Intersects(worldAABB))
                {
                    visibleStatic.push_back(go);
                    go->isVisibleInFrustum = true;
                    go->culledLastFrame = false;
                    renderedCount++;
                }
                else
                {
                    go->isVisibleInFrustum = false;
                    go->culledLastFrame = true;
                    culledCount++;
                }
            }
        }
        else
        {
            //if frustum is not active all are visible
            quadtree.GetAllObjects(visibleStatic);
            quadtreeTestsCount = 0;
            quadtreeCulledCount = 0;
            renderedCount += visibleStatic.size();

            for (GameObject* go : visibleStatic)
            {
                go->isVisibleInFrustum = true;
                go->culledLastFrame = false;
            }
        }
    }
    else if (!staticObjects.empty())
    {
        //if quadtree is not active we test frustum against all
        if (frustum != nullptr)
        {
            for (GameObject* go : staticObjects)
            {
                if (go->mesh->meshIndex >= (int)g_Meshes.size())
                    continue;

                WorldAABB worldAABB = go->mesh->GetWorldAABB();

                if (frustum->Intersects(worldAABB))
                {
                    visibleStatic.push_back(go);
                    go->isVisibleInFrustum = true;
                    go->culledLastFrame = false;
                    renderedCount++;
                }
                else
                {
                    go->isVisibleInFrustum = false;
                    go->culledLastFrame = true;
                    culledCount++;
                }
            }
        }
        else
        {
            visibleStatic = staticObjects;
            renderedCount += visibleStatic.size();
            for (GameObject* go : visibleStatic)
            {
                go->isVisibleInFrustum = true;
                go->culledLastFrame = false;
            }
        }
    }

    //process dynamic
    std::vector<GameObject*> visibleDynamic;

    for (GameObject* go : dynamicObjects)
    {
        go->isVisibleInFrustum = true;

        if (frustum != nullptr && go->mesh->meshIndex < (int)g_Meshes.size())
        {
            WorldAABB worldAABB = go->mesh->GetWorldAABB();

            //if the object is OUT we skip it
            if (!frustum->Intersects(worldAABB))
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

        if (go->isVisibleInFrustum)
        {
            visibleDynamic.push_back(go);
        }
    }

    //combine visible static and dynamic
    std::vector<GameObject*> allVisibleObjects;
    allVisibleObjects.insert(allVisibleObjects.end(), visibleStatic.begin(), visibleStatic.end());
    allVisibleObjects.insert(allVisibleObjects.end(), visibleDynamic.begin(), visibleDynamic.end());

    //filter transparent and opaque
    std::vector<GameObject*> opaqueObjects;
    std::vector<GameObject*> transparentObjects;

    for (GameObject* go : allVisibleObjects)
    {
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
    this->quadtreeCulledCount = quadtreeCulledCount;

    //render
    if (!debugZBuffer)
    {
        ModuleEditor* editor = Application::GetInstance().editor.get();

        // Resize selection texture and depth buffer
        if (editor && (selectionTextureWidth != viewportWidth || selectionTextureHeight != viewportHeight))
        {
            selectionTextureWidth = viewportWidth;
            selectionTextureHeight = viewportHeight;

            // Resize color texture
            glBindTexture(GL_TEXTURE_2D, selectionTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, viewportWidth, viewportHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, 0);

            // Resize depthbuffer
            glBindRenderbuffer(GL_RENDERBUFFER, selectionDepthBuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, viewportWidth, viewportHeight);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            std::cout << "Selection FBO resized to: " << viewportWidth << "x" << viewportHeight << std::endl;
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

        std::sort(transparentObjects.begin(), transparentObjects.end(),
            [](GameObject* a, GameObject* b) {
                return a->distanceToCamera > b->distanceToCamera;
            });

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glEnable(GL_DEPTH_TEST);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);

        for (GameObject* go : transparentObjects)
        {
            go->mesh->Draw(&camera);
        }

        glDisable(GL_POLYGON_OFFSET_FILL);
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

            float baseThickness = 2.0f;
            float scaleFactor = sqrtf((viewportWidth + viewportHeight) / 2.0f / 800.0f);
            float scaledThickness = baseThickness * scaleFactor;
            scaledThickness = glm::clamp(scaledThickness, 1.0f, 5.0f);

            glUniform1f(glGetUniformLocation(edgeDetectionShader, "outlineThickness"), scaledThickness);
            glUniform2f(glGetUniformLocation(edgeDetectionShader, "texelSize"),
                1.0f / viewportWidth, 1.0f / viewportHeight);

            glBindVertexArray(fullscreenQuadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        // Draw Octree debug visualization
        if (useQuadtree && showQuadtree)
        {
            quadtree.DebugDraw();
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

    if (selectionDepthBuffer) {
        glDeleteRenderbuffers(1, &selectionDepthBuffer);
        selectionDepthBuffer = 0;
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

bool OpenGL::EmptyQuadtree()
{
    //method to know if the quadtree is empty or not (if there are any static objects or not)
    glm::vec3 sceneMin(FLT_MAX);
    glm::vec3 sceneMax(-FLT_MAX);
    int staticCount = 0;

    for (GameObject* go : gameObjects)
    {
        if (go != nullptr && go->isStatic && go->mesh != nullptr && go->mesh->meshIndex >= 0)
        {
            WorldAABB worldAABB = go->mesh->GetWorldAABB();

            sceneMin.x = std::min(sceneMin.x, worldAABB.min.x);
            sceneMin.y = std::min(sceneMin.y, worldAABB.min.y);
            sceneMin.z = std::min(sceneMin.z, worldAABB.min.z);

            sceneMax.x = std::max(sceneMax.x, worldAABB.max.x);
            sceneMax.y = std::max(sceneMax.y, worldAABB.max.y);
            sceneMax.z = std::max(sceneMax.z, worldAABB.max.z);

            staticCount++;
        }
    }
    if (staticCount == 0)
    {
        quadtree.Clear();
        return true;
    }
    else return false;
}

void OpenGL::RebuildQuadtree()
{
    LOG("Rebuilding Octree...");

    //calculate aabb that contains all static
    glm::vec3 sceneMin(FLT_MAX);
    glm::vec3 sceneMax(-FLT_MAX);

    int staticCount = 0;

    for (GameObject* go : gameObjects)
    {
        if (go != nullptr && go->isStatic && go->mesh != nullptr && go->mesh->meshIndex >= 0)
        {
            WorldAABB worldAABB = go->mesh->GetWorldAABB();

            sceneMin.x = std::min(sceneMin.x, worldAABB.min.x);
            sceneMin.y = std::min(sceneMin.y, worldAABB.min.y);
            sceneMin.z = std::min(sceneMin.z, worldAABB.min.z);

            sceneMax.x = std::max(sceneMax.x, worldAABB.max.x);
            sceneMax.y = std::max(sceneMax.y, worldAABB.max.y);
            sceneMax.z = std::max(sceneMax.z, worldAABB.max.z);

            staticCount++;
        }
    }

    if (staticCount == 0)
    {
        LOG_WARNING("No static objects found to build Octree");
        quadtree.Clear();
        return;
    }

    float padding = 5.0f;

    glm::vec3 boundaryMin(sceneMin.x - padding, sceneMin.y - padding, sceneMin.z - padding);
    glm::vec3 boundaryMax(sceneMax.x + padding, sceneMax.y + padding, sceneMax.z + padding);

    AABB boundary(boundaryMin, boundaryMax);

    quadtree.Create(boundary, 4, 5);

    //insert game objects
    int insertedCount = 0;
    for (GameObject* go : gameObjects)
    {
        if (go != nullptr && go->isStatic && go->mesh != nullptr && go->mesh->meshIndex >= 0)
        {
            if (quadtree.Insert(go))
            {
                insertedCount++;
            }
        }
    }

    LOG("Octree rebuilt with " + std::to_string(insertedCount) + " static objects");
    LOG("Boundary: Min(" + std::to_string(sceneMin.x) + ", " + std::to_string(sceneMin.z) +
        ") Max(" + std::to_string(sceneMax.x) + ", " + std::to_string(sceneMax.z) + ")");
}