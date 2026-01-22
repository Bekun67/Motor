#include "InspectorWindow.h"
#include <imgui.h>
#include "Application.h"
#include "GameObject.h"
#include "SelectionManager.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ComponentCamera.h"
#include "ComponentRotate.h"
#include "ComponentRigidBody.h"
#include "ResourceTexture.h"
#include "ComponentCollider.h"
#include "ComponentFirstPersonController.h"
#include "Log.h"

InspectorWindow::InspectorWindow()
    : EditorWindow("Inspector")
{
}

void InspectorWindow::Draw()
{
    if (!isOpen) return;

    ImGui::Begin(name.c_str(), &isOpen);

    isHovered = (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows));

    SelectionManager* selectionManager = Application::GetInstance().selectionManager;

    if (!selectionManager->HasSelection())
    {
        ImGui::TextDisabled("No GameObject selected");
        ImGui::End();
        return;
    }

    GameObject* selectedObject = selectionManager->GetSelectedObject();

    if (selectedObject == nullptr)
    {
        ImGui::TextDisabled("Invalid selection");
        ImGui::End();
        return;
    }

    ImGui::Text("GameObject: %s", selectedObject->GetName().c_str());
    ImGui::SameLine();

    if (selectedObject->GetComponent(ComponentType::CAMERA)) {
        ComponentCamera* selectedCamera = static_cast<ComponentCamera*>(selectedObject->GetComponent(ComponentType::CAMERA));
        ComponentCamera* currentSceneCamera = Application::GetInstance().camera->GetSceneCamera();

        bool isActive = (selectedCamera == currentSceneCamera);

        if (ImGui::Checkbox("##isActive", &isActive)) {
            if (isActive) {
                Application::GetInstance().camera->SetSceneCamera(selectedCamera);
            }
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Set as Active Game Camera");
        }
    }

    ImGui::Separator();

    bool objectDeleted = DrawGameObjectSection(selectedObject);
    if (objectDeleted)
    {
        ImGui::End();
        return;
    }

    ImGui::Separator();
    DrawGizmoSettings();
    ImGui::Separator();

    DrawTransformComponent(selectedObject);
    DrawCameraComponent(selectedObject);
    DrawMeshComponent(selectedObject);
    DrawMaterialComponent(selectedObject);
    DrawRotateComponent(selectedObject);
    DrawRigidBodyComponent(selectedObject);
    DrawColliderComponent(selectedObject);
    DrawConstraintComponents(selectedObject);
    DrawFirstPersonControllerComponent(selectedObject);

    ImGui::End();
}

void InspectorWindow::DrawGizmoSettings()
{
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Gizmo Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Gizmo Mode:");
        ImGui::SameLine();
        ImGui::Text("Turn off (Q)");

        bool isTranslate = (currentGizmoOperation == ImGuizmo::TRANSLATE);
        bool isRotate = (currentGizmoOperation == ImGuizmo::ROTATE);
        bool isScale = (currentGizmoOperation == ImGuizmo::SCALE);

        if (ImGui::RadioButton("Translate (W)", isTranslate))
        {
            currentGizmoOperation = ImGuizmo::TRANSLATE;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Move the object in 3D space\nShortcut: W key");

        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate (E)", isRotate))
        {
            currentGizmoOperation = ImGuizmo::ROTATE;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Rotate the object\nShortcut: E key");

        ImGui::SameLine();
        if (ImGui::RadioButton("Scale (R)", isScale))
        {
            currentGizmoOperation = ImGuizmo::SCALE;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Scale the object\nShortcut: R key");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Transform Space (T):");
        ImGui::Spacing();

        bool isWorld = (currentGizmoMode == ImGuizmo::WORLD);
        bool isLocal = (currentGizmoMode == ImGuizmo::LOCAL);

        if (ImGui::RadioButton("World Space", isWorld))
        {
            currentGizmoMode = ImGuizmo::WORLD;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "World Space");
            ImGui::Separator();
            ImGui::Text("Transformations are relative to world axes");
            ImGui::BulletText("X: Always points right");
            ImGui::BulletText("Y: Always points up");
            ImGui::BulletText("Z: Always points forward");
            ImGui::EndTooltip();
        }

        ImGui::SameLine();
        if (ImGui::RadioButton("Local Space", isLocal))
        {
            currentGizmoMode = ImGuizmo::LOCAL;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Local Space");
            ImGui::Separator();
            ImGui::Text("Transformations are relative to objects rotation");
            ImGui::BulletText("Axes follow objects orientation");
            ImGui::BulletText("Useful for moving along objects direction");
            ImGui::EndTooltip();
        }
    }
}

void InspectorWindow::DrawTransformComponent(GameObject* selectedObject)
{
    Transform* transform = static_cast<Transform*>(selectedObject->GetComponent(ComponentType::TRANSFORM));

    if (transform == nullptr) return;

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        glm::vec3 position = transform->GetPosition();
        glm::vec3 rotation = transform->GetRotation();
        glm::vec3 scale = transform->GetScale();

        bool transformChanged = false;
        bool wasEditing = false;

        ComponentRigidBody* rb = static_cast<ComponentRigidBody*>(
            selectedObject->GetComponent(ComponentType::RIGIDBODY)
            );
        bool anyItemActive = false;
        static bool wasManipulating = false;

        ImGui::Text("Position");
        ImGui::PushItemWidth(80);
        ImGui::Text("X"); ImGui::SameLine(20);
        if (ImGui::DragFloat("##PosX", &position.x, 0.1f)) transformChanged = true;
        if (ImGui::IsItemActive()) anyItemActive = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::SameLine(120);
        ImGui::Text("Y"); ImGui::SameLine(130);
        if (ImGui::DragFloat("##PosY", &position.y, 0.1f)) transformChanged = true;
        if (ImGui::IsItemActive()) anyItemActive = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::SameLine(230);
        ImGui::Text("Z"); ImGui::SameLine(240);
        if (ImGui::DragFloat("##PosZ", &position.z, 0.1f)) transformChanged = true;
        if (ImGui::IsItemActive()) anyItemActive = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::Spacing();

        ImGui::Text("Rotation");
        ImGui::Text("X"); ImGui::SameLine(20);
        if (ImGui::DragFloat("##RotX", &rotation.x, 0.1f)) transformChanged = true;
        if (ImGui::IsItemActive()) anyItemActive = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::SameLine(120);
        ImGui::Text("Y"); ImGui::SameLine(130);
        if (ImGui::DragFloat("##RotY", &rotation.y, 0.1f)) transformChanged = true;
        if (ImGui::IsItemActive()) anyItemActive = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::SameLine(230);
        ImGui::Text("Z"); ImGui::SameLine(240);
        if (ImGui::DragFloat("##RotZ", &rotation.z, 0.1f)) transformChanged = true;
        if (ImGui::IsItemActive()) anyItemActive = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::Spacing();

        ImGui::Text("Scale");
        ImGui::Text("X"); ImGui::SameLine(20);
        if (ImGui::DragFloat("##ScaleX", &scale.x, 0.1f)) transformChanged = true;
        if (ImGui::IsItemActive()) anyItemActive = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::SameLine(120);
        ImGui::Text("Y"); ImGui::SameLine(130);
        if (ImGui::DragFloat("##ScaleY", &scale.y, 0.1f)) transformChanged = true;
        if (ImGui::IsItemActive()) anyItemActive = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::SameLine(230);
        ImGui::Text("Z"); ImGui::SameLine(240);
        if (ImGui::DragFloat("##ScaleZ", &scale.z, 0.1f)) transformChanged = true;
        if (ImGui::IsItemActive()) anyItemActive = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::PopItemWidth();

        if (rb && rb->IsActive())
        {
            if (anyItemActive && !wasManipulating)
            {
                rb->SetManipulating(true);
                wasManipulating = true;
            }
            else if (!anyItemActive && wasManipulating)
            {
                rb->SetManipulating(false);
                wasManipulating = false;
            }
        }
        else
        {
            wasManipulating = false;
        }

        if (transformChanged)
        {
            transform->SetPosition(position);
            transform->SetRotation(rotation);
            transform->SetScale(scale);
        }

        if (wasEditing)
        {
            Application::GetInstance().scene->MarkOctreeForRebuild();
            LOG_DEBUG("Octree rebuild requested after editing transform");
        }

        ImGui::Spacing();

        if (ImGui::Button("Reset Transform"))
        {
            transform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
            transform->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
            transform->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));

            Application::GetInstance().scene->MarkOctreeForRebuild();

            LOG_DEBUG("Transform reset for: %s", selectedObject->GetName().c_str());
            LOG_CONSOLE("Transform reset for: %s", selectedObject->GetName().c_str());
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Reset position to (0,0,0), rotation to (0,0,0), and scale to (1,1,1)");
        }
    }
}

void InspectorWindow::DrawCameraComponent(GameObject* selectedObject)
{
    ComponentCamera* cameraComp = static_cast<ComponentCamera*>(selectedObject->GetComponent(ComponentType::CAMERA));

    if (cameraComp == nullptr) return;

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Camera Settings");
        ImGui::Separator();

        float fov = cameraComp->GetFov();
        if (ImGui::SliderFloat("Field of View", &fov, 20.0f, 120.0f, "%.1f"))
        {
            cameraComp->SetFov(fov);
            LOG_DEBUG("Camera FOV set to: %.1f", fov);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Camera field of view in degrees");

        ImGui::Spacing();

        float nearPlane = cameraComp->GetNearPlane();
        float farPlane = cameraComp->GetFarPlane();

        if (ImGui::DragFloat("Near Plane", &nearPlane, 0.01f, 0.01f, 10.0f, "%.2f"))
        {
            cameraComp->SetNearPlane(nearPlane);
            LOG_DEBUG("Camera near plane set to: %.2f", nearPlane);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Near clipping plane distance");

        if (ImGui::DragFloat("Far Plane", &farPlane, 1.0f, 10.0f, 1000.0f, "%.1f"))
        {
            cameraComp->SetFarPlane(farPlane);
            LOG_DEBUG("Camera far plane set to: %.1f", farPlane);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Far clipping plane distance");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Text("Optimization Settings");

        bool frustumEnabled = cameraComp->IsFrustumCullingEnabled();
        if (ImGui::Checkbox("Enable Frustum Culling", &frustumEnabled))
        {
            cameraComp->SetFrustumCulling(frustumEnabled);
            LOG_DEBUG("Frustum culling %s for camera: %s",
                frustumEnabled ? "enabled" : "disabled",
                selectedObject->GetName().c_str());
            LOG_CONSOLE("Frustum culling %s for %s",
                frustumEnabled ? "enabled" : "disabled",
                selectedObject->GetName().c_str());
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Frustum Culling");
            ImGui::Separator();
            ImGui::Text("When enabled, objects outside the camera's");
            ImGui::Text("view frustum will not be rendered.");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Benefits:");
            ImGui::BulletText("Better performance in large scenes");
            ImGui::BulletText("Reduces GPU workload");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Status:");
            ImGui::BulletText(frustumEnabled ? "Objects outside view are HIDDEN" : "All objects are RENDERED");
            ImGui::EndTooltip();
        }

        ImGui::Spacing();

        bool drawFrustum = cameraComp->ShouldDrawFrustum();
        if (ImGui::Checkbox("Draw Frustum Gizmo", &drawFrustum))
        {
            cameraComp->SetDrawFrustum(drawFrustum);
            LOG_DEBUG("Frustum visualization %s for camera: %s",
                drawFrustum ? "enabled" : "disabled",
                selectedObject->GetName().c_str());
            LOG_CONSOLE("Frustum gizmo %s for %s",
                drawFrustum ? "enabled" : "disabled",
                selectedObject->GetName().c_str());
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Frustum Visualization");
            ImGui::Separator();
            ImGui::Text("Shows the camera's view frustum as a wireframe");
            ImGui::Text("in the 3D viewport.");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Color Coding:");
            ImGui::BulletText("Green: Active camera");
            ImGui::BulletText("Yellow: Inactive cameras");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Tip:");
            ImGui::Text("Use this to debug what each camera sees");
            ImGui::EndTooltip();
        }

        ImGui::Spacing();

        if (frustumEnabled)
        {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "? Culling Active");
            if (drawFrustum)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "| ?? Frustum Visible");
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "? Culling Disabled");
            if (drawFrustum)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "| ?? Frustum Visible");
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
    }
}

void InspectorWindow::DrawMeshComponent(GameObject* selectedObject)
{
    ComponentMesh* meshComp = static_cast<ComponentMesh*>(selectedObject->GetComponent(ComponentType::MESH));

    if (meshComp == nullptr) return;

    if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Mesh:");
        ImGui::SameLine();

        // Mesh display
        std::string currentMeshName = "None";
        if (meshComp->HasMesh() && meshComp->IsUsingResourceMesh())
        {
            UID currentUID = meshComp->GetMeshUID();
            ModuleResources* resources = Application::GetInstance().resources.get();
            const Resource* res = resources->GetResourceDirect(currentUID);
            if (res)
            {
                currentMeshName = std::string(res->GetAssetFile());
                // Filename
                size_t lastSlash = currentMeshName.find_last_of("/\\");
                if (lastSlash != std::string::npos)
                    currentMeshName = currentMeshName.substr(lastSlash + 1);
            }
            else
            {
                //Show UID
                currentMeshName = "UID " + std::to_string(currentUID);
            }
        }
        else if (meshComp->HasMesh() && meshComp->IsUsingDirectMesh())
        {
            currentMeshName = "[Primitive]";
        }

        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##MeshSelector", currentMeshName.c_str()))
        {
            // Get mesh resources
            ModuleResources* resources = Application::GetInstance().resources.get();
            const std::map<UID, Resource*>& allResources = resources->GetAllResources();

            for (const auto& pair : allResources)
            {
                const Resource* res = pair.second;
                if (res->GetType() == Resource::MESH)
                {
                    std::string meshName = res->GetAssetFile();

                    // Filename
                    size_t lastSlash = meshName.find_last_of("/\\");
                    if (lastSlash != std::string::npos) meshName = meshName.substr(lastSlash + 1);

                    UID meshUID = res->GetUID();
                    bool isSelected = (meshComp->IsUsingResourceMesh() && meshComp->GetMeshUID() == meshUID);

                    std::string displayName = meshName;
                    if (res->IsLoadedToMemory())
                    {
                        displayName += " [Loaded]";
                    }

                    if (ImGui::Selectable(displayName.c_str(), isSelected))
                    {
                        if (meshComp->LoadMeshByUID(meshUID))
                        {
                            LOG_DEBUG("Assigned mesh '%s' (UID %llu) to GameObject '%s'",
                                meshName.c_str(), meshUID, selectedObject->GetName().c_str());
                            LOG_CONSOLE("Mesh '%s' assigned to '%s'",
                                meshName.c_str(), selectedObject->GetName().c_str());
                        }
                        else
                        {
                            LOG_CONSOLE("Failed to load mesh '%s' (UID %llu)", meshName.c_str(), meshUID);
                        }
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus(); // Highlight selected item
                    }

                    // Show tooltip with UID and path
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("UID: %llu", meshUID);
                        ImGui::Text("Path: %s", res->GetAssetFile());
                        if (res->IsLoadedToMemory())
                        {
                            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Loaded in memory");
                        }
                        ImGui::EndTooltip();
                    }
                }
            }

            ImGui::EndCombo();
        }

        if (meshComp->HasMesh())
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            const Mesh& mesh = meshComp->GetMesh();

            ImGui::Text("Mesh Statistics:");
            ImGui::Text("Vertices: %d", (int)mesh.vertices.size());
            ImGui::Text("Indices: %d", (int)mesh.indices.size());
            ImGui::Text("Triangles: %d", (int)mesh.indices.size() / 3);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Normals Visualization:");

            if (ImGui::Checkbox("Show Vertex Normals", &showVertexNormals))
            {
                LOG_DEBUG("Vertex normals visualization: %s", showVertexNormals ? "ON" : "OFF");
            }

            if (ImGui::Checkbox("Show Face Normals", &showFaceNormals))
            {
                LOG_DEBUG("Face normals visualization: %s", showFaceNormals ? "ON" : "OFF");
            }
        }
    }
}

void InspectorWindow::DrawMaterialComponent(GameObject* selectedObject)
{
    ComponentMaterial* materialComp = static_cast<ComponentMaterial*>(selectedObject->GetComponent(ComponentType::MATERIAL));

    if (materialComp == nullptr) return;

    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Texture:");
        ImGui::SameLine();

        std::string currentTextureName = "None";

        if (materialComp->IsUsingCheckerboard()) {
            currentTextureName = "[Checkerboard Pattern]";
        }
        else if (materialComp->HasTexture())
        {
            UID currentUID = materialComp->GetTextureUID();
            ModuleResources* resources = Application::GetInstance().resources.get();
            const Resource* res = resources->GetResourceDirect(currentUID);
            if (res)
            {
                currentTextureName = std::string(res->GetAssetFile());
                size_t lastSlash = currentTextureName.find_last_of("/\\");
                if (lastSlash != std::string::npos)
                    currentTextureName = currentTextureName.substr(lastSlash + 1);
            }
            else
            {
                currentTextureName = "UID " + std::to_string(currentUID);
            }
        }

        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##TextureSelector", currentTextureName.c_str()))
        {
            bool isCheckerboardSelected = materialComp->IsUsingCheckerboard();
            if (ImGui::Selectable("[Checkerboard Pattern]", isCheckerboardSelected))
            {
                materialComp->CreateCheckerboardTexture();
                LOG_DEBUG("Applied checkerboard texture to: %s", selectedObject->GetName().c_str());
                LOG_CONSOLE("Checkerboard texture applied to %s", selectedObject->GetName().c_str());
            }

            if (isCheckerboardSelected)
            {
                ImGui::SetItemDefaultFocus();
            }

            ImGui::Separator();

            ModuleResources* resources = Application::GetInstance().resources.get();
            const std::map<UID, Resource*>& allResources = resources->GetAllResources();

            for (const auto& pair : allResources)
            {
                const Resource* res = pair.second;
                if (res->GetType() == Resource::TEXTURE)
                {
                    std::string textureName = res->GetAssetFile();

                    size_t lastSlash = textureName.find_last_of("/\\");
                    if (lastSlash != std::string::npos) textureName = textureName.substr(lastSlash + 1);

                    UID textureUID = res->GetUID();
                    bool isSelected = (!materialComp->IsUsingCheckerboard() &&
                        materialComp->HasTexture() &&
                        materialComp->GetTextureUID() == textureUID);

                    std::string displayName = textureName;
                    if (res->IsLoadedToMemory())
                    {
                        displayName += " [Loaded]";
                    }

                    const ResourceTexture* texRes = static_cast<const ResourceTexture*>(res);
                    unsigned int gpuID = texRes->GetGPU_ID();

                    if (ImGui::Selectable(displayName.c_str(), isSelected))
                    {
                        if (materialComp->LoadTextureByUID(textureUID))
                        {
                            LOG_DEBUG("Assigned texture '%s' (UID %llu) to GameObject '%s'",
                                textureName.c_str(), textureUID, selectedObject->GetName().c_str());
                            LOG_CONSOLE("Texture '%s' assigned to '%s'",
                                textureName.c_str(), selectedObject->GetName().c_str());
                        }
                        else
                        {
                            LOG_CONSOLE("Failed to load texture '%s' (UID %llu)", textureName.c_str(), textureUID);
                        }
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();

                        if (gpuID != 0)
                        {
                            float tooltipPreviewSize = 128.0f;
                            float width = (float)texRes->GetWidth();
                            float height = (float)texRes->GetHeight();
                            float scale = tooltipPreviewSize / std::max(width, height);
                            ImVec2 tooltipSize(width * scale, height * scale);

                            ImGui::Image((ImTextureID)(intptr_t)gpuID, tooltipSize);
                            ImGui::Separator();
                        }

                        ImGui::Text("%s", textureName.c_str());
                        ImGui::Text("UID: %llu", textureUID);
                        ImGui::Text("Size: %d x %d", texRes->GetWidth(), texRes->GetHeight());
                        ImGui::Text("Path: %s", res->GetAssetFile());
                        if (res->IsLoadedToMemory())
                        {
                            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Loaded in memory");
                        }
                        else
                        {
                            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Not loaded");
                        }
                        ImGui::EndTooltip();
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (materialComp->HasTexture())
        {
            unsigned int gpuID = 0;
            int texWidth = 0;
            int texHeight = 0;

            if (materialComp->IsUsingCheckerboard())
            {
                Renderer* renderer = Application::GetInstance().renderer.get();
                if (renderer)
                {
                    Texture* defaultTex = renderer->GetDefaultTexture();
                    if (defaultTex)
                    {
                        gpuID = defaultTex->GetID();
                        texWidth = defaultTex->GetWidth();
                        texHeight = defaultTex->GetHeight();
                    }
                }
            }
            else
            {
                UID currentUID = materialComp->GetTextureUID();
                ModuleResources* resources = Application::GetInstance().resources.get();
                const Resource* res = resources->GetResourceDirect(currentUID);

                if (res && res->GetType() == Resource::TEXTURE)
                {
                    const ResourceTexture* texRes = static_cast<const ResourceTexture*>(res);
                    gpuID = texRes->GetGPU_ID();
                    texWidth = texRes->GetWidth();
                    texHeight = texRes->GetHeight();
                }
            }

            if (gpuID != 0)
            {
                ImGui::Text("Texture Preview:");

                float previewMaxSize = 256.0f;
                float width = (float)texWidth;
                float height = (float)texHeight;

                float scale = previewMaxSize / std::max(width, height);
                ImVec2 previewSize(width * scale, height * scale);

                float windowWidth = ImGui::GetContentRegionAvail().x;
                float offsetX = (windowWidth - previewSize.x) * 0.5f;
                if (offsetX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

                ImGui::Image((ImTextureID)(intptr_t)gpuID, previewSize);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            }

            ImGui::Text("Size: %d x %d pixels", materialComp->GetTextureWidth(), materialComp->GetTextureHeight());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Actions:");
        ImGui::Spacing();

        if (ImGui::Button("Apply Checkerboard", ImVec2(-1, 0)))
        {
            materialComp->CreateCheckerboardTexture();
            LOG_DEBUG("Applied checkerboard texture to: %s", selectedObject->GetName().c_str());
            LOG_CONSOLE("Checkerboard texture applied to %s", selectedObject->GetName().c_str());
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Applies the default black and white checkerboard pattern");
        }

        ImGui::Spacing();

        if (materialComp->HasOriginalTexture())
        {
            if (ImGui::Button("Restore Original", ImVec2(-1, 0)))
            {
                materialComp->RestoreOriginalTexture();
                LOG_DEBUG("Restored original texture to: %s", selectedObject->GetName().c_str());
                LOG_CONSOLE("Original texture restored to %s", selectedObject->GetName().c_str());
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Restores the original texture that was previously loaded");
            }
        }
    }
}

void InspectorWindow::DrawRotateComponent(GameObject* selectedObject)
{
    ComponentRotate* rotateComp = static_cast<ComponentRotate*>(selectedObject->GetComponent(ComponentType::ROTATE));

    if (rotateComp == nullptr) return;

    if (ImGui::CollapsingHeader("Auto Rotate", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool active = rotateComp->IsActive();
        if (ImGui::Checkbox("Enable Auto Rotation", &active))
        {
            rotateComp->SetActive(active);
        }

        rotateComp->OnEditor();
    }
}

bool InspectorWindow::DrawGameObjectSection(GameObject* selectedObject)
{
    bool objectDeleted = false;

    if (ImGui::CollapsingHeader("GameObject", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Actions:");
        ImGui::Spacing();

        // Delete button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

        if (ImGui::Button("Delete GameObject", ImVec2(-1, 0)))
        {
            if (selectedObject != Application::GetInstance().scene->GetRoot())
            {
                selectedObject->MarkForDeletion();
                LOG_DEBUG("GameObject '%s' marked for deletion", selectedObject->GetName().c_str());
                LOG_CONSOLE("GameObject '%s' marked for deletion", selectedObject->GetName().c_str());

                Application::GetInstance().selectionManager->ClearSelection();
                objectDeleted = true;
            }
            else
            {
                LOG_CONSOLE("Cannot delete Root GameObject!");
            }
        }

        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Delete GameObject");
            ImGui::Separator();
            ImGui::Text("Marks this GameObject for deletion");
            ImGui::Text("Shortcut: Backspace key");
            ImGui::EndTooltip();
        }

        ImGui::Spacing();

        // Create empty child button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));

        if (ImGui::Button("Create Empty Child", ImVec2(-1, 0)))
        {
            GameObject* newChild = Application::GetInstance().scene->CreateGameObject("Empty");
            newChild->SetParent(selectedObject);

            Application::GetInstance().selectionManager->SetSelectedObject(newChild);

            LOG_DEBUG("Created empty child for '%s'", selectedObject->GetName().c_str());
            LOG_CONSOLE("Created empty child '%s' under '%s'", newChild->GetName().c_str(), selectedObject->GetName().c_str());
        }

        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Create Empty Child");
            ImGui::Separator();
            ImGui::Text("Creates a new empty GameObject as a child");
            ImGui::Text("of this GameObject");
            ImGui::EndTooltip();
        }
    }

    return objectDeleted;
}

void InspectorWindow::GetAllGameObjects(GameObject* root, std::vector<GameObject*>& outObjects)
{
    if (root == nullptr)
        return;

    outObjects.push_back(root);

    const std::vector<GameObject*>& children = root->GetChildren();
    for (GameObject* child : children)
    {
        GetAllGameObjects(child, outObjects);
    }
}

bool InspectorWindow::IsDescendantOf(GameObject* potentialDescendant, GameObject* potentialAncestor)
{
    if (potentialDescendant == nullptr || potentialAncestor == nullptr)
        return false;

    GameObject* current = potentialDescendant->GetParent();
    while (current != nullptr)
    {
        if (current == potentialAncestor)
            return true;
        current = current->GetParent();
    }

    return false;
}

void InspectorWindow::DrawRigidBodyComponent(GameObject* selectedObject)
{
    ComponentRigidBody* rigidBody = static_cast<ComponentRigidBody*>(selectedObject->GetComponent(ComponentType::RIGIDBODY));

    if (rigidBody == nullptr)
        return;

    ComponentFirstPersonController* fpc = static_cast<ComponentFirstPersonController*>(
        selectedObject->GetComponent(ComponentType::FIRSTPERSON)
        );

    if (fpc != nullptr)
        return;

    if (ImGui::CollapsingHeader("RigidBody", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushID("RigidBody");

        // Active checkbox
        bool isActive = rigidBody->IsActive();
        if (ImGui::Checkbox("Active##RigidBody", &isActive))
        {
            rigidBody->SetActive(isActive);
        }

        ImGui::Separator();

        // Mass
        float mass = rigidBody->GetMass();
        if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.0f, 1000.0f))
        {
            rigidBody->SetMass(mass);
        }

        // Kinematic
        bool isKinematic = rigidBody->IsKinematic();
        if (ImGui::Checkbox("Is Kinematic", &isKinematic))
        {
            rigidBody->SetKinematic(isKinematic);
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Kinematic objects are not affected by forces");
            ImGui::Text("but can still collide with other objects");
            ImGui::EndTooltip();
        }

        ImGui::Separator();

        // Velocity (read-only)
        glm::vec3 velocity = rigidBody->GetVelocity();
        ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", velocity.x, velocity.y, velocity.z);

        ImGui::Spacing();

        // Apply force buttons (only when playing)
        Application::PlayState playState = Application::GetInstance().GetPlayState();
        bool isPlaying = (playState == Application::PlayState::PLAYING);

        if (!isPlaying)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Apply Upward Force"))
        {
            rigidBody->ApplyForce(glm::vec3(0.0f, 10.0f, 0.0f));
        }

        ImGui::SameLine();

        if (ImGui::Button("Apply Forward Impulse"))
        {
            rigidBody->ApplyImpulse(glm::vec3(0.0f, 0.0f, 5.0f));
        }

        if (!isPlaying)
        {
            ImGui::EndDisabled();
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f),
                "Physics controls only available in Play mode");
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

        if (ImGui::Button("Remove Rigid Body", ImVec2(-1, 0)))
        {
            std::vector<Component*> constraints = selectedObject->GetComponentsOfType(ComponentType::CONSTRAINT);
            for (Component* comp : constraints)
            {
                selectedObject->RemoveComponent(comp);
            }

            selectedObject->RemoveComponent(rigidBody);
            ImGui::PopStyleColor(3);
            ImGui::PopID();
            return;
        }

        ImGui::Separator();
        ImGui::PopStyleColor(3);

        ImGui::PopID();
    }
}

void InspectorWindow::DrawColliderComponent(GameObject* selectedObject)
{
    std::vector<Component*> colliders = selectedObject->GetComponentsOfType(ComponentType::COLLIDER);

    if (colliders.empty())
        return;

	// Verify if in play mode
    ComponentFirstPersonController* fpc = static_cast<ComponentFirstPersonController*>(
        selectedObject->GetComponent(ComponentType::FIRSTPERSON)
        );

    Application::PlayState playState = Application::GetInstance().GetPlayState();
    bool isPlaying = (playState == Application::PlayState::PLAYING);

    //draw each collider
    for (size_t colliderIndex = 0; colliderIndex < colliders.size(); ++colliderIndex)
    {
        ComponentCollider* collider = static_cast<ComponentCollider*>(colliders[colliderIndex]);

        if (collider == nullptr)
            continue;

        if (fpc && collider->GetColliderType() == ColliderType::SPHERE)
        {
            continue;
        }

        ImGui::PushID(static_cast<int>(colliderIndex));

        std::string headerName = collider->GetColliderTypeName() + " Collider";

        if (ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            //active checkbox
            bool isActive = collider->IsActive();
            if (ImGui::Checkbox("Active##Collider", &isActive))
            {
                collider->SetActive(isActive);
            }

            ImGui::SameLine();

            //show Debug checkbox
            bool showDebug = collider->GetShowDebug();
            if (ImGui::Checkbox("Show Debug##Collider", &showDebug))
            {
                collider->SetShowDebug(showDebug);
                LOG_DEBUG("Collider debug visualization %s for '%s'",
                    showDebug ? "enabled" : "disabled",
                    selectedObject->GetName().c_str());
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Visualize collider shape in the scene view");
            }

            ImGui::Separator();

            //collider type
            const char* colliderTypes[] = { "Box", "Sphere", "Cylinder", "Capsule", "Plane", "Mesh" };
            int currentType = static_cast<int>(collider->GetColliderType());

            ImGui::Text("Collider Type:");

            if (isPlaying)
            {
                ImGui::BeginDisabled();
            }

            if (ImGui::Combo("##ColliderType", &currentType, colliderTypes, IM_ARRAYSIZE(colliderTypes)))
            {
                collider->SetColliderType(static_cast<ColliderType>(currentType));
                LOG_DEBUG("Changed collider type to %s for '%s'",
                    colliderTypes[currentType],
                    selectedObject->GetName().c_str());
            }

            if (isPlaying)
            {
                ImGui::EndDisabled();

				// Show tooltip when disabled explanation
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                {
                    ImGui::BeginTooltip();
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f),
                        "Cannot change collider type during Play mode");
                    ImGui::Text("Stop the simulation to modify collider type");
                    ImGui::EndTooltip();
                }
            }
            else if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Changing collider type will reset manual edits");
                ImGui::Text("and recalculate size from mesh bounds");
                ImGui::EndTooltip();
            }

            ImGui::Separator();

            ColliderType type = collider->GetColliderType();

            switch (type)
            {
            case ColliderType::BOX:
            {
                glm::vec3 size = collider->GetBoxSize();
                if (ImGui::DragFloat3("Size", &size.x, 0.1f, 0.01f, 100.0f))
                {
                    collider->SetBoxSize(size);
                }
                break;
            }

            case ColliderType::SPHERE:
            {
                float radius = collider->GetSphereRadius();
                if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.01f, 100.0f))
                {
                    collider->SetSphereRadius(radius);
                }
                break;
            }

            case ColliderType::CYLINDER:
            {
                float radius = collider->GetCylinderRadius();
                float height = collider->GetCylinderHeight();

                if (ImGui::DragFloat("Radius##Cylinder", &radius, 0.1f, 0.01f, 100.0f))
                {
                    collider->SetCylinderSize(radius, height);
                }

                if (ImGui::DragFloat("Height##Cylinder", &height, 0.1f, 0.01f, 100.0f))
                {
                    collider->SetCylinderSize(radius, height);
                }
                break;
            }

            case ColliderType::CAPSULE:
            {
                float radius = collider->GetCapsuleRadius();
                float height = collider->GetCapsuleHeight();

                if (ImGui::DragFloat("Radius##Capsule", &radius, 0.1f, 0.01f, 100.0f))
                {
                    collider->SetCapsuleSize(radius, height);
                }

                if (ImGui::DragFloat("Height##Capsule", &height, 0.1f, 0.01f, 100.0f))
                {
                    collider->SetCapsuleSize(radius, height);
                }
                break;
            }

            case ColliderType::PLANE:
            {
                glm::vec3 size = collider->GetBoxSize();

                ImGui::Text("Plane Dimensions:");

                float width = size.x;
                float height = glm::max(size.y, size.z);

                if (ImGui::DragFloat("Width", &width, 0.1f, 0.01f, 100.0f))
                {
                    size.x = width;
                    collider->SetBoxSize(size);
                }

                if (ImGui::DragFloat("Height", &height, 0.1f, 0.01f, 100.0f))
                {
                    size.y = height;
                    size.z = height;
                    collider->SetBoxSize(size);
                }

                break;
            }

            case ColliderType::MESH:
            {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
                    "Convex Hull Mesh Collider");
                ImGui::Text("Automatically fits mesh geometry");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                break;
            }
            }

            ImGui::Separator();

            //offset
            ImGui::Text("Offset:");
            glm::vec3 displayOffset = collider->GetUserOffset();
            if (ImGui::DragFloat3("##Offset", &displayOffset.x, 0.1f, -100.0f, 100.0f))
            {
                collider->SetOffset(displayOffset);
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Position offset from GameObject's transform");
            }

            ImGui::SameLine();
            if (ImGui::Button("Reset##OffsetReset"))
            {
                collider->SetOffset(glm::vec3(0.0f, 0.0f, 0.0f));
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Reset offset to (0, 0, 0)");
            }

            ImGui::Separator();

            //trigger checkbox
            bool isTrigger = collider->IsTrigger();
            if (ImGui::Checkbox("Is Trigger", &isTrigger))
            {
                collider->SetIsTrigger(isTrigger);
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Trigger colliders don't cause physical collisions");
                ImGui::Text("but still detect overlaps for game logic");
                ImGui::EndTooltip();
            }

            ImGui::Separator();

            //physics mat
            ImGui::Text("Physics Material:");

            float friction = collider->GetFriction();
            if (ImGui::SliderFloat("Friction", &friction, 0.0f, 1.0f))
            {
                collider->SetFriction(friction);
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Surface friction (0 = slippery, 1 = rough)");
            }

            float restitution = collider->GetRestitution();
            if (ImGui::SliderFloat("Bounciness", &restitution, 0.0f, 1.0f))
            {
                collider->SetRestitution(restitution);
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Bounciness (0 = no bounce, 1 = perfect bounce)");
            }

            ImGui::Separator();
        }


        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

        if (ImGui::Button("Remove Collider", ImVec2(-1, 0)))
        {
            selectedObject->RemoveComponent(collider);
            ImGui::PopStyleColor(3);
            ImGui::PopID();
            return;
        }

        ImGui::Separator();
        ImGui::PopStyleColor(3);

        ImGui::PopID();
    }
}

void InspectorWindow::DrawConstraintComponents(GameObject* selectedObject)
{
    std::vector<Component*> constraints = selectedObject->GetComponentsOfType(ComponentType::CONSTRAINT);

    if (constraints.empty())
        return;

    // Check if in play mode
    Application::PlayState playState = Application::GetInstance().GetPlayState();
    bool isPlaying = (playState == Application::PlayState::PLAYING);

    // Draw each constraint
    for (size_t constraintIndex = 0; constraintIndex < constraints.size(); ++constraintIndex)
    {
        ComponentConstraint* baseConstraint = static_cast<ComponentConstraint*>(constraints[constraintIndex]);

        if (baseConstraint == nullptr)
            continue;

        ImGui::PushID(static_cast<int>(constraintIndex) + 1000); // Offset to avoid ID conflicts

        if (!baseConstraint->IsConstraintValid())
        {
            if (baseConstraint->GetConnectedBody() != nullptr)
            {
                baseConstraint->OnConnectedBodyInvalidated();
                LOG_DEBUG("[InspectorWindow] Forced constraint to world on '%s'", selectedObject->GetName().c_str());
            }
        }

        ConstraintType type = baseConstraint->GetConstraintType();
        std::string headerName;

        // Determine header name based on constraint type
        switch (type)
        {
        case ConstraintType::HINGE:
            headerName = "Hinge Constraint";
            break;
        case ConstraintType::SLIDER:
            headerName = "Slider Constraint";
            break;
        case ConstraintType::DISTANCE:
            headerName = "Distance Constraint";
            break;
        case ConstraintType::CONE:
            headerName = "Cone Constraint";
            break;
        default:
            headerName = "Unknown Constraint";
            break;
        }

        if (ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Active checkbox
            bool isActive = baseConstraint->IsActive();
            if (ImGui::Checkbox("Active##Constraint", &isActive))
            {
                baseConstraint->SetActive(isActive);
            }

            ImGui::Separator();

            // Connected Body selection
            ImGui::Text("Connected Body:");
            GameObject* connectedBody = baseConstraint->GetConnectedBody();
            std::string connectedBodyName = connectedBody ? connectedBody->GetName() : "None (World)";

            ImGui::SetNextItemWidth(-1);
            if (ImGui::BeginCombo("##ConnectedBody", connectedBodyName.c_str()))
            {
                // Option for "None" (attach to world)
                bool isNone = (connectedBody == nullptr);
                if (ImGui::Selectable("None (World)", isNone))
                {
                    baseConstraint->SetConnectedBody(nullptr);
                }

                if (isNone)
                    ImGui::SetItemDefaultFocus();

                // Get all GameObjects with RigidBody
                std::vector<GameObject*> allObjects;
                GetAllGameObjects(Application::GetInstance().scene->GetRoot(), allObjects);

                for (GameObject* obj : allObjects)
                {
                    // Skip self
                    if (obj == selectedObject)
                        continue;

                    // Only show objects with RigidBody
                    if (obj->GetComponent(ComponentType::RIGIDBODY) == nullptr)
                        continue;

                    bool isSelected = (connectedBody == obj);
                    if (ImGui::Selectable(obj->GetName().c_str(), isSelected))
                    {
                        baseConstraint->SetConnectedBody(obj);
                    }

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Select the RigidBody to connect to.\nChoose 'None' to attach to world.");
            }

            ImGui::Separator();

            // Anchor points
            ImGui::Text("Anchor Points:");

            glm::vec3 anchorA = baseConstraint->GetAnchorPointA();
            if (ImGui::DragFloat3("Anchor A (Local)", &anchorA.x, 0.1f))
            {
                baseConstraint->SetAnchorPointA(anchorA);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Anchor point in local space of this object");

            if (connectedBody)
            {
                glm::vec3 anchorB = baseConstraint->GetAnchorPointB();
                if (ImGui::DragFloat3("Anchor B (Local)", &anchorB.x, 0.1f))
                {
                    baseConstraint->SetAnchorPointB(anchorB);
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Anchor point in local space of connected body");
            }

            ImGui::Separator();

            // Draw type-specific settings
            switch (type)
            {
            case ConstraintType::HINGE:
                DrawHingeConstraintSettings(static_cast<ComponentHingeConstraint*>(baseConstraint), isPlaying);
                break;
            case ConstraintType::SLIDER:
                DrawSliderConstraintSettings(static_cast<ComponentSliderConstraint*>(baseConstraint), isPlaying);
                break;
            case ConstraintType::DISTANCE:
                DrawDistanceConstraintSettings(static_cast<ComponentDistanceConstraint*>(baseConstraint), isPlaying);
                break;
            case ConstraintType::CONE:
                DrawConeConstraintSettings(static_cast<ComponentConeConstraint*>(baseConstraint), isPlaying);
                break;
            }

            ImGui::Separator();

            // Common settings
            ImGui::Text("Common Settings:");

            bool constraintEnabled = baseConstraint->IsConstraintEnabled();
            if (ImGui::Checkbox("Constraint Enabled", &constraintEnabled))
            {
                baseConstraint->SetConstraintEnabled(constraintEnabled);
            }

            float breakingThreshold = baseConstraint->GetBreakingThreshold();
            if (ImGui::DragFloat("Breaking Threshold", &breakingThreshold, 0.1f, 0.0f, 10000.0f))
            {
                baseConstraint->SetBreakingThreshold(breakingThreshold);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Force required to break the constraint");
                ImGui::Text("0 = Unbreakable");
                ImGui::EndTooltip();
            }

            ImGui::Separator();

            // Remove button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

            if (ImGui::Button("Remove Constraint", ImVec2(-1, 0)))
            {
                selectedObject->RemoveComponent(baseConstraint);
                ImGui::PopStyleColor(3);
                ImGui::PopID();
                return;
            }

            ImGui::PopStyleColor(3);
        }

        ImGui::Separator();
        ImGui::PopID();
    }
}

void InspectorWindow::DrawHingeConstraintSettings(ComponentHingeConstraint* hinge, bool isPlaying)
{
    ImGui::Text("Hinge Settings:");
    ImGui::Spacing();

    // Axis
    glm::vec3 axisA = hinge->GetAxisA();
    if (ImGui::DragFloat3("Axis A", &axisA.x, 0.01f, -1.0f, 1.0f))
    {
        hinge->SetAxisA(axisA);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Rotation axis in local space of object A");

    if (hinge->GetConnectedBody())
    {
        glm::vec3 axisB = hinge->GetAxisB();
        if (ImGui::DragFloat3("Axis B", &axisB.x, 0.01f, -1.0f, 1.0f))
        {
            hinge->SetAxisB(axisB);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Rotation axis in local space of object B");
    }

    ImGui::Separator();

    // Angle limits
    bool useLimits = hinge->GetUseLimits();
    if (ImGui::Checkbox("Use Angle Limits", &useLimits))
    {
        hinge->SetUseLimits(useLimits);
    }

    if (useLimits)
    {
        ImGui::Indent();

        float lowLimit = glm::degrees(hinge->GetLowLimit());
        float highLimit = glm::degrees(hinge->GetHighLimit());

        if (ImGui::SliderFloat("Low Limit (deg)", &lowLimit, -180.0f, 180.0f))
        {
            hinge->SetLimits(glm::radians(lowLimit), glm::radians(highLimit));
        }

        if (ImGui::SliderFloat("High Limit (deg)", &highLimit, -180.0f, 180.0f))
        {
            hinge->SetLimits(glm::radians(lowLimit), glm::radians(highLimit));
        }

        ImGui::Unindent();

        // Show current angle if playing
        if (isPlaying)
        {
            float currentAngle = glm::degrees(hinge->GetCurrentAngle());
            ImGui::Text("Current Angle: %.2f°", currentAngle);

            // Visual indicator
            float normalizedAngle = (currentAngle - lowLimit) / (highLimit - lowLimit);
            ImGui::ProgressBar(normalizedAngle, ImVec2(-1, 0));
        }
    }

    ImGui::Separator();

    // Motor
    bool useMotor = hinge->GetUseMotor();
    if (ImGui::Checkbox("Use Motor", &useMotor))
    {
        hinge->SetUseMotor(useMotor);
    }

    if (useMotor)
    {
        ImGui::Indent();

        float motorVelocity = hinge->GetMotorVelocity();
        if (ImGui::DragFloat("Motor Velocity", &motorVelocity, 0.1f, -100.0f, 100.0f))
        {
            hinge->SetMotorVelocity(motorVelocity);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Target angular velocity (rad/s)");

        float motorMaxImpulse = hinge->GetMotorMaxImpulse();
        if (ImGui::DragFloat("Max Motor Force", &motorMaxImpulse, 0.1f, 0.0f, 1000.0f))
        {
            hinge->SetMotorMaxImpulse(motorMaxImpulse);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Maximum force the motor can apply");

        ImGui::Unindent();
    }
}

void InspectorWindow::DrawSliderConstraintSettings(ComponentSliderConstraint* slider, bool isPlaying)
{
    ImGui::Text("Slider Settings:");
    ImGui::Spacing();

    // Axis
    glm::vec3 axisA = slider->GetAxisA();
    if (ImGui::DragFloat3("Slide Axis A", &axisA.x, 0.01f, -1.0f, 1.0f))
    {
        slider->SetAxisA(axisA);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Sliding axis in local space of object A");

    if (slider->GetConnectedBody())
    {
        glm::vec3 axisB = slider->GetAxisB();
        if (ImGui::DragFloat3("Slide Axis B", &axisB.x, 0.01f, -1.0f, 1.0f))
        {
            slider->SetAxisB(axisB);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Sliding axis in local space of object B");
    }

    ImGui::Separator();

    // Linear limits
    bool useLinearLimits = slider->GetUseLinearLimits();
    if (ImGui::Checkbox("Use Linear Limits", &useLinearLimits))
    {
        slider->SetUseLinearLimits(useLinearLimits);
    }

    if (useLinearLimits)
    {
        ImGui::Indent();

        float lowerLimit = slider->GetLowerLinearLimit();
        float upperLimit = slider->GetUpperLinearLimit();

        if (ImGui::DragFloat("Lower Limit", &lowerLimit, 0.1f, -100.0f, 100.0f))
        {
            slider->SetLinearLimits(lowerLimit, upperLimit);
        }

        if (ImGui::DragFloat("Upper Limit", &upperLimit, 0.1f, -100.0f, 100.0f))
        {
            slider->SetLinearLimits(lowerLimit, upperLimit);
        }

        ImGui::Unindent();

        // Show current position if playing
        if (isPlaying)
        {
            float currentPos = slider->GetCurrentLinearPosition();
            ImGui::Text("Current Position: %.2f", currentPos);

            // Visual indicator
            float normalizedPos = (currentPos - lowerLimit) / (upperLimit - lowerLimit);
            ImGui::ProgressBar(normalizedPos, ImVec2(-1, 0));
        }
    }

    ImGui::Separator();

    // Angular limits
    bool useAngularLimits = slider->GetUseAngularLimits();
    if (ImGui::Checkbox("Use Angular Limits", &useAngularLimits))
    {
        slider->SetUseAngularLimits(useAngularLimits);
    }

    if (useAngularLimits)
    {
        ImGui::Indent();

        float lowerAngular = glm::degrees(slider->GetLowerAngularLimit());
        float upperAngular = glm::degrees(slider->GetUpperAngularLimit());

        if (ImGui::SliderFloat("Lower Angular (deg)", &lowerAngular, -180.0f, 180.0f))
        {
            slider->SetAngularLimits(glm::radians(lowerAngular), glm::radians(upperAngular));
        }

        if (ImGui::SliderFloat("Upper Angular (deg)", &upperAngular, -180.0f, 180.0f))
        {
            slider->SetAngularLimits(glm::radians(lowerAngular), glm::radians(upperAngular));
        }

        ImGui::Unindent();

        // Show current angular position if playing
        if (isPlaying)
        {
            float currentAngular = glm::degrees(slider->GetCurrentAngularPosition());
            ImGui::Text("Current Angle: %.2f°", currentAngular);
        }
    }
}

void InspectorWindow::DrawDistanceConstraintSettings(ComponentDistanceConstraint* distance, bool isPlaying)
{
    ImGui::Text("Distance Settings:");
    ImGui::Spacing();

    // Distance
    float dist = distance->GetDistance();
    if (ImGui::DragFloat("Target Distance", &dist, 0.1f, 0.0f, 100.0f))
    {
        distance->SetDistance(dist);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Desired distance between anchor points");

    // Show current distance if playing
    if (isPlaying && distance->GetConnectedBody())
    {
        float currentDist = distance->GetCurrentDistance();
        ImGui::Text("Current Distance: %.2f", currentDist);

        // Color based on deviation from target
        float deviation = glm::abs(currentDist - dist);
        ImVec4 color = deviation < 0.1f ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.7f, 0.3f, 1.0f);
        ImGui::TextColored(color, "Deviation: %.2f", deviation);
    }

    ImGui::Separator();

    // Stiffness
    float stiffness = distance->GetStiffness();
    if (ImGui::SliderFloat("Stiffness", &stiffness, 0.0f, 1.0f))
    {
        distance->SetStiffness(stiffness);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("How rigid the distance constraint is");
        ImGui::BulletText("0.0 = Loose spring");
        ImGui::BulletText("1.0 = Rigid connection");
        ImGui::EndTooltip();
    }

    // Damping
    float damping = distance->GetDamping();
    if (ImGui::SliderFloat("Damping", &damping, 0.0f, 1.0f))
    {
        distance->SetDamping(damping);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Resistance to oscillation (0 = bouncy, 1 = damped)");

    ImGui::Separator();

    // Min/Max distance limits
    bool useMinDist = distance->GetUseMinDistance();
    if (ImGui::Checkbox("Use Min Distance", &useMinDist))
    {
        distance->SetUseMinDistance(useMinDist);
    }

    if (useMinDist)
    {
        ImGui::Indent();
        float minDist = distance->GetMinDistance();
        if (ImGui::DragFloat("Min Distance", &minDist, 0.1f, 0.0f, 100.0f))
        {
            distance->SetMinDistance(minDist);
        }
        ImGui::Unindent();
    }

    bool useMaxDist = distance->GetUseMaxDistance();
    if (ImGui::Checkbox("Use Max Distance", &useMaxDist))
    {
        distance->SetUseMaxDistance(useMaxDist);
    }

    if (useMaxDist)
    {
        ImGui::Indent();
        float maxDist = distance->GetMaxDistance();
        if (ImGui::DragFloat("Max Distance", &maxDist, 0.1f, 0.0f, 100.0f))
        {
            distance->SetMaxDistance(maxDist);
        }
        ImGui::Unindent();
    }
}

void InspectorWindow::DrawConeConstraintSettings(ComponentConeConstraint* cone, bool isPlaying)
{
    ImGui::Text("Cone Constraint Settings:");
    ImGui::Spacing();

    // Axis
    glm::vec3 axisA = cone->GetAxisA();
    if (ImGui::DragFloat3("Cone Axis A", &axisA.x, 0.01f, -1.0f, 1.0f))
    {
        cone->SetAxisA(axisA);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Cone axis in local space of object A");

    if (cone->GetConnectedBody())
    {
        glm::vec3 axisB = cone->GetAxisB();
        if (ImGui::DragFloat3("Cone Axis B", &axisB.x, 0.01f, -1.0f, 1.0f))
        {
            cone->SetAxisB(axisB);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Cone axis in local space of object B");
    }

    ImGui::Separator();

    // Swing limits
    bool useSwingLimits = cone->GetUseSwingLimits();
    if (ImGui::Checkbox("Use Swing Limits", &useSwingLimits))
    {
        cone->SetUseSwingLimits(useSwingLimits);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Limit the cone angle (ball-and-socket movement)");

    if (useSwingLimits)
    {
        ImGui::Indent();

        float swingSpan1 = glm::degrees(cone->GetSwingSpan1());
        if (ImGui::SliderFloat("Swing Span Y (deg)", &swingSpan1, 0.0f, 180.0f))
        {
            cone->SetSwingSpan1(glm::radians(swingSpan1));
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Maximum swing angle around Y axis");

        float swingSpan2 = glm::degrees(cone->GetSwingSpan2());
        if (ImGui::SliderFloat("Swing Span Z (deg)", &swingSpan2, 0.0f, 180.0f))
        {
            cone->SetSwingSpan2(glm::radians(swingSpan2));
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Maximum swing angle around Z axis");

        ImGui::Unindent();
    }

    ImGui::Separator();

    // Twist limits
    bool useTwistLimits = cone->GetUseTwistLimits();
    if (ImGui::Checkbox("Use Twist Limits", &useTwistLimits))
    {
        cone->SetUseTwistLimits(useTwistLimits);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Limit rotation around the cone axis");

    if (useTwistLimits)
    {
        ImGui::Indent();

        float twistSpan = glm::degrees(cone->GetTwistSpan());
        if (ImGui::SliderFloat("Twist Span (deg)", &twistSpan, 0.0f, 180.0f))
        {
            cone->SetTwistSpan(glm::radians(twistSpan));
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Maximum twist angle around cone axis");

        ImGui::Unindent();
    }

    ImGui::Separator();

    // Advanced settings
    if (ImGui::CollapsingHeader("Advanced Settings"))
    {
        ImGui::Indent();

        float softness = cone->GetLimitSoftness();
        if (ImGui::SliderFloat("Limit Softness", &softness, 0.0f, 1.0f))
        {
            cone->SetLimitSoftness(softness);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("How soft the limits are (0 = hard, 1 = soft)");

        float bias = cone->GetLimitBias();
        if (ImGui::SliderFloat("Limit Bias", &bias, 0.0f, 1.0f))
        {
            cone->SetLimitBias(bias);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Error correction factor for limits");

        float relaxation = cone->GetLimitRelaxation();
        if (ImGui::SliderFloat("Limit Relaxation", &relaxation, 0.0f, 1.0f))
        {
            cone->SetLimitRelaxation(relaxation);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("How much the limit can be violated");

        ImGui::Unindent();
    }
}

void InspectorWindow::DrawFirstPersonControllerComponent(GameObject* selectedObject)
{
    ComponentFirstPersonController* fpc = static_cast<ComponentFirstPersonController*>(
        selectedObject->GetComponent(ComponentType::FIRSTPERSON)
        );

    if (fpc == nullptr)
        return;

    // Verify if in play mode
    Application::PlayState playState = Application::GetInstance().GetPlayState();
    bool isPlaying = (playState == Application::PlayState::PLAYING);

    if (ImGui::CollapsingHeader("First Person Controller", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushID("FirstPersonController");

        // Active checkbox
        bool isActive = fpc->IsActive();
        if (ImGui::Checkbox("Active##FPC", &isActive))
        {
            fpc->SetActive(isActive);
            LOG_DEBUG("FirstPersonController active state changed to: %s", isActive ? "true" : "false");
        }

        ImGui::Separator();

        // Movement Settings
        ImGui::Text("Movement Settings:");
        ImGui::Spacing();

        float movementSpeed = fpc->GetMovementSpeed();
        if (ImGui::DragFloat("Movement Speed", &movementSpeed, 0.1f, 0.1f, 50.0f))
        {
            fpc->SetMovementSpeed(movementSpeed);
            LOG_DEBUG("Movement speed changed to: %.2f", movementSpeed);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Speed of camera movement (WASD)");
        }

        float mouseSensitivity = fpc->GetMouseSensitivity();
        if (ImGui::DragFloat("Mouse Sensitivity", &mouseSensitivity, 0.01f, 0.01f, 2.0f))
        {
            fpc->SetMouseSensitivity(mouseSensitivity);
            LOG_DEBUG("Mouse sensitivity changed to: %.2f", mouseSensitivity);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Sensitivity for mouse look (hold right mouse button)");
        }

        ImGui::Separator();

        // Projectile Settings
        ImGui::Text("Player Collider:");
        ImGui::Spacing();

        ComponentCollider* collider = static_cast<ComponentCollider*>(
            selectedObject->GetComponent(ComponentType::COLLIDER)
            );

        if (collider && collider->GetColliderType() == ColliderType::SPHERE)
        {
            bool colliderActive = collider->IsActive();
            if (ImGui::Checkbox("Collider Active", &colliderActive))
            {
                collider->SetActive(colliderActive);
            }

            ImGui::SameLine();

            bool showDebug = collider->GetShowDebug();
            if (ImGui::Checkbox("Show Debug", &showDebug))
            {
                collider->SetShowDebug(showDebug);
            }

            float colliderRadius = fpc->GetColliderRadius();
            if (ImGui::DragFloat("Collider Radius", &colliderRadius, 0.05f, 0.1f, 5.0f))
            {
                fpc->SetColliderRadius(colliderRadius);
                LOG_DEBUG("Collider radius changed to: %.2f", colliderRadius);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Radius of the sphere collider around the player");
            }

            float friction = collider->GetFriction();
            if (ImGui::SliderFloat("Friction", &friction, 0.0f, 1.0f))
            {
                collider->SetFriction(friction);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Surface friction (0 = slippery, 1 = rough)");
            }

            float restitution = collider->GetRestitution();
            if (ImGui::SliderFloat("Bounciness", &restitution, 0.0f, 1.0f))
            {
                collider->SetRestitution(restitution);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Bounciness (0 = no bounce, 1 = perfect bounce)");
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No sphere collider found!");
            ImGui::Text("Add a sphere collider to enable player collision");
        }

        ImGui::Separator();

        ImGui::Text("Physics Settings:");
        ImGui::Spacing();

        ComponentRigidBody* rb = static_cast<ComponentRigidBody*>(
            selectedObject->GetComponent(ComponentType::RIGIDBODY)
            );

        if (rb)
        {
            float mass = rb->GetMass();
            if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.1f, 100.0f))
            {
                rb->SetMass(mass);
                LOG_DEBUG("Player mass changed to: %.2f", mass);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Mass of the player (affects physics interactions)");
            }

            glm::vec3 velocity = rb->GetVelocity();
            ImGui::Text("Current Velocity: (%.2f, %.2f, %.2f)", velocity.x, velocity.y, velocity.z);

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Current movement velocity (read-only)");
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No RigidBody found!");
        }

        ImGui::Separator();

        ImGui::Text("Projectile Settings:");
        ImGui::Spacing();

        float shootForce = fpc->GetShootForce();
        if (ImGui::DragFloat("Shoot Force", &shootForce, 0.5f, 1.0f, 200.0f))
        {
            fpc->SetShootForce(shootForce);
            LOG_DEBUG("Shoot force changed to: %.2f", shootForce);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Force applied to fired spheres (left click to shoot)");
        }

        float sphereSize = fpc->GetSphereSize();
        if (ImGui::DragFloat("Sphere Size", &sphereSize, 0.05f, 0.1f, 5.0f))
        {
            fpc->SetSphereSize(sphereSize);
            LOG_DEBUG("Sphere size changed to: %.2f", sphereSize);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Size of fired spheres");
        }

        ImGui::Separator();

        // Controls info
        if (isPlaying)
        {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "✓ Controls Active");
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "⚠ Controls (Play Mode Only)");
        }

        ImGui::Spacing();
        ImGui::Indent();
        ImGui::BulletText("WASD - Move camera");
        ImGui::BulletText("Space/Ctrl - Move up/down");
        ImGui::BulletText("Right Mouse - Look around");
        ImGui::BulletText("Left Click - Shoot sphere");
        ImGui::Unindent();

        ImGui::Separator();

        // Remove button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

        if (ImGui::Button("Remove First Person Controller", ImVec2(-1, 0)))
        {
            ComponentRigidBody* rb = static_cast<ComponentRigidBody*>(
                selectedObject->GetComponent(ComponentType::RIGIDBODY)
                );
            if (rb)
            {
                selectedObject->RemoveComponent(rb);
                LOG_DEBUG("Removed RigidBody from '%s'", selectedObject->GetName().c_str());
            }

            ComponentCollider* collider = static_cast<ComponentCollider*>(
                selectedObject->GetComponent(ComponentType::COLLIDER)
                );
            if (collider)
            {
                selectedObject->RemoveComponent(collider);
                LOG_DEBUG("Removed Collider from '%s'", selectedObject->GetName().c_str());
            }

            selectedObject->RemoveComponent(fpc);
            LOG_CONSOLE("Removed First Person Controller (with RigidBody and Collider) from '%s'", selectedObject->GetName().c_str());

            ImGui::PopStyleColor(3);
            ImGui::PopID();
            return;
        }

        ImGui::PopStyleColor(3);
        ImGui::Separator();

        ImGui::PopID();
    }
}