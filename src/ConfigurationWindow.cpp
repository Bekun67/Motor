#include "ConfigurationWindow.h"
#include "ModuleEditor.h"
#include "Application.h"
#include "Window.h"
#include "OpenGL.h"
#include "Camera.h"
#include "LoadFBX.h"
#include <IL/il.h>

ConfigurationWindow::ConfigurationWindow(ModuleEditor* editor)
    : EditorWindow(editor, "Configuration")
{
}

void ConfigurationWindow::Draw()
{
    if (!visible) return;

    if (editor->firstTimeSetup)
    {
        ImGui::SetNextWindowPos(ImVec2(400, 150), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_FirstUseEver);
    }

    ImGui::Begin("Configuration", &visible);

    if (ImGui::CollapsingHeader("Editor Layout", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextWrapped("Reset all ImGui windows to their default positions and sizes.");
        ImGui::Spacing();

        if (ImGui::Button("Reset Layout", ImVec2(-1, 0)))
        {
            editor->ResetLayout();
            LOG("Resetting editor layout to default positions");
        }

        ImGui::Spacing();
        ImGui::Checkbox("Adaptive Layout", &editor->useAdaptiveLayout);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Automatically adjust layout when window is resized");
        }

        ImGui::Spacing();
        ImGui::Separator();
    }

    if (ImGui::CollapsingHeader("Application", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // FPS Graph
        if (!editor->fpsHistory.empty())
        {
            float avgFPS = 0.0f;
            for (float fps : editor->fpsHistory)
                avgFPS += fps;
            avgFPS /= editor->fpsHistory.size();

            char title[64];
            sprintf(title, "Framerate %.1f FPS", avgFPS);
            ImGui::PlotHistogram("##framerate", editor->fpsHistory.data(), (int)editor->fpsHistory.size(),
                0, title, 0.0f, 120.0f, ImVec2(0, 80));
        }

        ImGui::Text("Frame Time: %.3f ms", editor->lastFrameTime * 1000.0f);
    }

    //frustum culling
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Camera* cam = &Application::GetInstance().opengl->camera;
        ComponentCamera* editorCam = cam->GetCameraComponent();

        if (editorCam)
        {
            // FOV
            float fov = editorCam->GetFOV();
            if (ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f))
            {
                editorCam->SetFOV(fov);
                LOG("Camera FOV changed to " + std::to_string(fov));
            }

            // Near Plane
            float nearPlane = editorCam->GetNearPlane();
            if (ImGui::DragFloat("Near Plane", &nearPlane, 0.1f, 0.1f, 10.0f))
            {
                editorCam->SetNearPlane(nearPlane);
                LOG("Camera Near Plane changed to " + std::to_string(nearPlane));
            }

            // Far Plane
            float farPlane = editorCam->GetFarPlane();
            if (ImGui::DragFloat("Far Plane", &farPlane, 1.0f, 10.0f, 10000.0f))
            {
                editorCam->SetFarPlane(farPlane);
                LOG("Camera Far Plane changed to " + std::to_string(farPlane));
            }

            ImGui::Separator();

            // Frustum Culling Toggle
            if (ImGui::Checkbox("Enable Frustum Culling", &cam->frustumCullingEnabled))
            {
                if (cam->frustumCullingEnabled)
                {
                    LOG("Frustum Culling ENABLED");
                }
                else
                {
                    LOG("Frustum Culling DISABLED");
                }
            }

            // Statistics
            if (cam->frustumCullingEnabled)
            {
                if (ImGui::Checkbox("Show Culling Statistics", &showCullingStats))
                {
                    if (showCullingStats)
                    {
                        LOG("Showing Frustum Culling Statistics");
                    }
                    else
                    {
                        LOG("Hiding Frustum Culling Statistics");
                    }
				}
                if (showCullingStats)
                {
                    OpenGL* opengl = Application::GetInstance().opengl.get();

                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Rendering Statistics:");

                    ImGui::Text("Objects Rendered: %d", opengl->renderedCount);

                    if (opengl->useQuadtree)
                    {
                        ImGui::Text("Culled by Frustum: %d", opengl->culledCount);
                        ImGui::Text("Culled by Octree: %d", opengl->quadtreeCulledCount);

                        int totalCulled = opengl->culledCount + opengl->quadtreeCulledCount;
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "TOTAL Culled: %d", totalCulled);

                        int totalObjects = opengl->renderedCount + totalCulled;
                        if (totalObjects > 0)
                        {
                            float cullingEfficiency = (float)totalCulled / (float)totalObjects * 100.0f;
                            ImGui::Text("Total Culling Efficiency: %.1f%%", cullingEfficiency);
                        }
                    }
                    else
                    {
                        ImGui::Text("Objects Culled: %d", opengl->culledCount);

                        int total = opengl->renderedCount + opengl->culledCount;
                        if (total > 0)
                        {
                            float percentage = (float)opengl->culledCount / (float)total * 100.0f;
                            ImGui::Text("Culling Efficiency: %.1f%%", percentage);
                        }
                    }
                }
                
            }

            // Debug Raycast Toggle
            if (ImGui::Checkbox("Show Raycast to Game Objects", &editorCam->debugRaycastEnabled))
            {
                if (editorCam->debugRaycastEnabled)
                {
                    LOG("Raycast to Game Objects ENABLED");
                }
                else
                {
                    LOG("Raycast to Game Objects DISABLED");
                }
            }

            // Z-Buffer Debug Toggle
            OpenGL* opengl = Application::GetInstance().opengl.get();
            if (ImGui::Checkbox("Show Z-Buffer Depth Debug", &opengl->debugZBuffer))
            {
                if (opengl->debugZBuffer)
                {
                    LOG("Z-Buffer ENABLED");
                }
                else
                {
                    LOG("Z-Buffer DISABLED");
                }
            }
        }
    }

    if (ImGui::CollapsingHeader("Window"))
    {
        Window* window = Application::GetInstance().window.get();
        if (window)
        {
            int width, height;
            window->GetWindowSize(width, height);
            ImGui::Text("Width: %d", width);
            ImGui::Text("Height: %d", height);
            ImGui::Text("Scale: %d", window->GetScale());
        }
    }

    if (ImGui::CollapsingHeader("Renderer"))
    {
        OpenGL* opengl = Application::GetInstance().opengl.get();
        if (opengl)
        {
            ImGui::Checkbox("Show Grid", &opengl->showGrid);

            //vertex normals
            if (ImGui::Checkbox("Show All Vertex Normals", &editor->showAllVertexNormals))
            {
                //apply to all GameObjects in scene
                for (GameObject* go : opengl->gameObjects)
                {
                    if (go && go->mesh)
                    {
                        go->mesh->showVertexNormals = editor->showAllVertexNormals;
                    }
                }

                if (editor->showAllAABBs) LOG("Enabled Vertex normals visualization for all GameObjects");
                else LOG("Disabled Vertex Normals visualization for all GameObjects");
            }

            //face normals
            if (ImGui::Checkbox("Show All Face Normals", &editor->showAllFaceNormals))
            {
                //apply to all GameObjects in scene
                for (GameObject* go : opengl->gameObjects)
                {
                    if (go && go->mesh)
                    {
                        go->mesh->showFaceNormals = editor->showAllFaceNormals;
                    }
                }

                if (editor->showAllAABBs) LOG("Enabled Face normals visualization for all GameObjects");
                else LOG("Disabled Face Normals visualization for all GameObjects");
            }

            //aabb viewer
            if (ImGui::Checkbox("Show All AABBs", &editor->showAllAABBs))
            {
                //apply to all GameObjects in scene
                for (GameObject* go : opengl->gameObjects)
                {
                    if (go && go->mesh)
                    {
                        go->mesh->showAABB = editor->showAllAABBs;
                    }
                }

                if (editor->showAllAABBs) LOG("Enabled AABB visualization for all GameObjects");
                else LOG("Disabled AABB visualization for all GameObjects");
            }
            ImGui::Text("GameObjects in scene: %zu", opengl->gameObjects.size());
        }
    }

    //octree section
    //quadtree section
    if (ImGui::CollapsingHeader("Space Partitioning", ImGuiTreeNodeFlags_DefaultOpen))
    {
        OpenGL* opengl = Application::GetInstance().opengl.get();
        if (opengl)
        {
            //change usage
            if (ImGui::Checkbox("Use Octree", &opengl->useQuadtree))
            {
                if (opengl->useQuadtree)
                {
                    opengl->RebuildQuadtree();
                    if (!opengl->EmptyQuadtree()) LOG("Octree enabled");
                }
                else
                {
                    opengl->quadtree.Clear();
                    LOG("Octree disabled");
                }
            }

            //if quadtree is active we add an option for showing
            if (opengl->useQuadtree)
            {
                if (ImGui::Checkbox("Show Octree", &opengl->showQuadtree))
                {
                    if (opengl->showQuadtree)
                    {
                        LOG("Octree Debug ENABLED");
                    }
                    else
                    {
                        LOG("Octree Debug DISABLED");
                    }
                }

                if (ImGui::Checkbox("Show Octree Statistics", &showOctreeStats))
                {
                    if (showOctreeStats)
                    {
                        LOG("Octree Statistics enabled");
                    }
                    else
                    {
                        LOG("Octree Statistics disabled");
                    }
                }

                if (ImGui::Button("Rebuild Octree", ImVec2(-1, 0)))
                {
                    opengl->RebuildQuadtree();
                    LOG("Octree rebuilt");
                }

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Octree Statistics:");

                std::vector<GameObject*> allInQuadtree;
                opengl->quadtree.GetAllObjects(allInQuadtree);
                ImGui::Text("Objects in Octree: %d", (int)allInQuadtree.size());

                //stats
                if (showOctreeStats && opengl->camera.frustumCullingEnabled)
                {
                    ImGui::Separator();
                    //octree stats with frustum
                    ImGui::Text("Frustum tests performed: %d", opengl->quadtreeTestsCount);
                    ImGui::Text("Discarded by Octree: %d", opengl->quadtreeCulledCount);
                    ImGui::Text("Discarded by Frustum: %d", opengl->culledCount);

                    int totalStatic = (int)allInQuadtree.size();
                    if (totalStatic > 0)
                    {
                        float quadtreeEfficiency = 100.0f * (float)opengl->quadtreeCulledCount / (float)totalStatic;
                        ImGui::ProgressBar(quadtreeEfficiency / 100.0f, ImVec2(-1, 0),
                            (std::to_string((int)quadtreeEfficiency) + "% skipped by Octree").c_str());

                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "WITHOUT Octree: %d frustum tests", totalStatic);
                        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "WITH Octree: %d frustum tests", opengl->quadtreeTestsCount);

                        int testsSaved = totalStatic - opengl->quadtreeTestsCount;
                        float savingsPercentage = 100.0f * (float)testsSaved / (float)totalStatic;
                        ImGui::Text("Tests Saved: %d (%.1f%%)", testsSaved, savingsPercentage);
                    }
                }

                ImGui::Separator();

                if (ImGui::Checkbox("Extra Octree LOGs", &opengl->extraQuadtreeInfo))
                {
                    if (opengl->extraQuadtreeInfo)
                    {
                        LOG("Extra Octree LOGs enabled");
                    }
                    else
                    {
                        LOG("Extra Octree LOGs disabled");
                    }
                }
            }
        }
    }

    if (ImGui::CollapsingHeader("Hardware"))
    {
        // Get current pc hardware and software versions
        const GLubyte* glVersion = glGetString(GL_VERSION);
        ImGui::Text("OpenGL Version: %s", glVersion);

        const GLubyte* glRenderer = glGetString(GL_RENDERER);
        ImGui::Text("GPU: %s", glRenderer);

        const GLubyte* glVendor = glGetString(GL_VENDOR);
        ImGui::Text("GPU Vendor: %s", glVendor);

        ILint devilVersion = ilGetInteger(IL_VERSION_NUM);
        ImGui::Text("DevIL Version: %d.%d.%d",
            devilVersion / 100,
            (devilVersion % 100) / 10,
            devilVersion % 10);
    }

    if (ImGui::CollapsingHeader("Memory"))
    {
        ImGui::Text("Total meshes loaded: %zu", g_Meshes.size());

        size_t totalVertices = 0;
        for (const auto& mesh : g_Meshes)
            totalVertices += mesh.numIndices;
        ImGui::Text("Total indices: %zu", totalVertices);
    }

    ImGui::End();
}