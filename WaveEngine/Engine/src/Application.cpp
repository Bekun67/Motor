#include "Application.h"
#include <iostream>
#include <chrono>

Application::Application() : isRunning(true), playState(PlayState::EDITING)
{
    LOG_DEBUG("=== Creating Application Instance ===");
    LOG_CONSOLE("Starting engine...");

    window = std::make_shared<Window>();
    input = std::make_shared<Input>();
    renderContext = std::make_shared<RenderContext>();
    renderer = std::make_shared<Renderer>();
    scene = std::make_shared<ModuleScene>();
    camera = std::make_shared<ModuleCamera>();
    editor = std::make_shared<ModuleEditor>();
    filesystem = std::make_shared<FileSystem>();
    time = std::make_shared<Time>();
    grid = std::make_shared<Grid>();
    resources = std::make_shared<ModuleResources>();
    physics = std::make_shared<ModulePhysics>(); 

    AddModule(std::static_pointer_cast<Module>(window));
    AddModule(std::static_pointer_cast<Module>(input));
    AddModule(std::static_pointer_cast<Module>(renderContext));
    AddModule(std::static_pointer_cast<Module>(scene));
    AddModule(std::static_pointer_cast<Module>(camera));
    AddModule(std::static_pointer_cast<Module>(editor));
    AddModule(std::static_pointer_cast<Module>(resources));
    AddModule(std::static_pointer_cast<Module>(filesystem));
    AddModule(std::static_pointer_cast<Module>(time));
    AddModule(std::static_pointer_cast<Module>(grid));
    AddModule(std::static_pointer_cast<Module>(physics));  
    AddModule(std::static_pointer_cast<Module>(renderer));

    selectionManager = new SelectionManager();
}

Application& Application::GetInstance()
{
    static Application instance;
    return instance;
}

void Application::AddModule(std::shared_ptr<Module> module)
{
    moduleList.push_back(module);
}

bool Application::Awake()
{
    return true;
}

bool Application::Start()
{
    LOG_CONSOLE("Starting engine modules...");
    LOG_CONSOLE("========================================");

    auto totalStart = std::chrono::high_resolution_clock::now();

    bool result = true;
    for (const auto& module : moduleList) {
        auto moduleStart = std::chrono::high_resolution_clock::now();

        LOG_CONSOLE("Initializing: %s...", module->name.c_str());

        result = module.get()->Start();

        auto moduleEnd = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(moduleEnd - moduleStart).count();

        LOG_CONSOLE("  -> %s: %lld ms", module->name.c_str(), duration);

        if (!result) {
            LOG_DEBUG("ERROR: Module failed to start: %s", module->name.c_str());
            LOG_CONSOLE("ERROR: Failed to initialize module: %s", module->name.c_str());
            break;
        }
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart).count();

    LOG_CONSOLE("========================================");
    LOG_CONSOLE("TOTAL INIT TIME: %lld ms", totalDuration);
    LOG_CONSOLE("========================================");

    if (result)
    {
        LOG_CONSOLE("Engine ready - All systems initialized");
    }

    return true;
}

bool Application::Update()
{
    // Check if exit was requested from menu first
    if (!isRunning) {
        LOG_DEBUG("Exit requested by user");
        LOG_CONSOLE("Shutting down...");
        return false;
    }

    bool ret = true;

    if (input->GetWindowEvent(WE_QUIT) == true) {
        LOG_DEBUG("Window close event detected");
        LOG_CONSOLE("Shutting down...");
        ret = false;
    }

    if (ret == true)
        ret = PreUpdate();

    if (ret == true)
        ret = DoUpdate();

    if (ret == true)
        ret = PostUpdate();

    return ret;
}

bool Application::PreUpdate()
{
    //Iterates the module list and calls PreUpdate on each module
    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->PreUpdate();
        if (!result) {
            break;
        }
    }

    return result;
}

// Call modules on each loop iteration
bool Application::DoUpdate()
{
    //Iterates the module list and calls Update on each module
    bool result = true;
    for (const auto& module : moduleList) {
        // Skip scene updates when in editing mode
        if (playState == PlayState::EDITING && module == scene) {
            continue;
        }

        result = module.get()->Update();
        if (!result) {
            break;
        }
    }

    return result;
}

// Call modules after each loop iteration
bool Application::PostUpdate()
{
    //Iterates the module list and calls PostUpdate on each module
    bool result = true;

    for (const auto& module : moduleList) {
        if (module == window) {
            continue;
        }

        result = module.get()->PostUpdate();
        if (!result) {
            break;
        }
    }

    if (result) {
        result = window->PostUpdate();
    }

    return result;
}

void Application::Play()
{
    // Save

    if (playState == PlayState::EDITING) {
        LOG_CONSOLE("Saving initial scene state...");

        GameObject* root = scene->GetRoot();
        if (root)
        {
            int index = 0;
            GameObject::AssignSerializationIndices(root, index);
            LOG_DEBUG("[Application] Assigned serialization indices to %d GameObjects", index);
        }

        scene->SaveScene("../Library/TempScene/__temp_scene_state__.json");
    }

    playState = PlayState::PLAYING;
    time->Resume();

    LOG_CONSOLE("Syncing physics to current transforms...");
    GameObject* root = scene->GetRoot();
    if (root)
    {
        DestroyConstraintsRecursive(root);

        SyncPhysicsRecursive(root);

        RecreateConstraintsRecursive(root);
    }
}

void Application::Stop()
{
    // Restore
    if (playState != PlayState::EDITING) {
        LOG_CONSOLE("Restoring initial scene state...");

        // Clean up physics objects before loading the saved scene
        GameObject* root = scene->GetRoot();
        if (root)
        {
            ValidateConstraintsRecursive(root);
        }

        if (root)
        {
            DestroyConstraintsRecursive(root);
        }

        ModulePhysics* physicsModule = physics.get();
        if (physicsModule && physicsModule->GetDynamicsWorld())
        {
            btDynamicsWorld* world = physicsModule->GetDynamicsWorld();

            int numConstraints = world->getNumConstraints();
            for (int i = numConstraints - 1; i >= 0; i--)
            {
                btTypedConstraint* constraint = world->getConstraint(i);
                world->removeConstraint(constraint);
                delete constraint;
            }
            LOG_CONSOLE("  -> Removed %d residual constraints", numConstraints);
        }

        if (root)
        {
            DisableRigidBodiesRecursive(root);
        }

        if (root)
        {
            DisableCollidersRecursive(root);
        }

        scene->LoadScene("../Library/TempScene/__temp_scene_state__.json");

        root = scene->GetRoot();
        if (root)
        {
            std::vector<GameObject*> allGameObjects;
            GameObject::CollectAllGameObjects(root, allGameObjects);
            LOG_DEBUG("[Application] Collected %zu GameObjects for constraint reference resolution", allGameObjects.size());

            GameObject::ResolveConstraintReferences(allGameObjects);
            LOG_DEBUG("[Application] Resolved constraint references after loading scene");
        }
    }

    playState = PlayState::EDITING;
    time->Reset();
    time->Pause();

}

void Application::Pause()
{
    playState = PlayState::PAUSED;
    time->Pause();
}

void Application::Step()
{
    if (playState != PlayState::EDITING)
    {
        time->StepFrame();
    }
}

bool Application::CleanUp()
{
    LOG_DEBUG("=== Cleaning Up Application ===");
    LOG_CONSOLE("Cleaning up modules...");

    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->CleanUp();
        if (!result) {
            break;
        }
    }
    moduleList.clear();

    editor.reset();
    camera.reset();
    scene.reset();
    renderer.reset();
    renderContext.reset();
    grid.reset();
    time.reset();
    filesystem.reset();
    input.reset();
    window.reset();
    resources.reset();

    delete selectionManager;
    selectionManager = nullptr;

    ConsoleLog::GetInstance().Shutdown();

    LOG_DEBUG("=== Application Cleanup Complete ===");
    LOG_CONSOLE("Shutdown complete");
    return result;
}

void Application::SyncPhysicsRecursive(GameObject* obj)
{
    if (!obj || !obj->IsActive()) return;

    ComponentRigidBody* rb = static_cast<ComponentRigidBody*>(
        obj->GetComponent(ComponentType::RIGIDBODY)
        );

    if (rb && rb->IsActive())
    {
        rb->SyncTransformToPhysics();
    }

    for (GameObject* child : obj->GetChildren())
    {
        SyncPhysicsRecursive(child);
    }
}

void Application::DestroyConstraintsRecursive(GameObject* obj)
{
    if (!obj) return;

	// Destroy all constraints of this GameObject
    std::vector<Component*> constraints = obj->GetComponentsOfType(ComponentType::CONSTRAINT);
    for (Component* comp : constraints)
    {
        ComponentConstraint* constraint = static_cast<ComponentConstraint*>(comp);
        if (constraint)
        {
			// Just destroy the constraint, do not remove the component
            constraint->DestroyConstraint();
            LOG_DEBUG("[Application] Destroyed constraint on '%s' before play", obj->GetName().c_str());
        }
    }

	// process children recursively
    for (GameObject* child : obj->GetChildren())
    {
        DestroyConstraintsRecursive(child);
    }
}

void Application::RecreateConstraintsRecursive(GameObject* obj)
{
    if (!obj) return;

	// Recreate all constraints of this GameObject
    std::vector<Component*> constraints = obj->GetComponentsOfType(ComponentType::CONSTRAINT);
    for (Component* comp : constraints)
    {
        ComponentConstraint* constraint = static_cast<ComponentConstraint*>(comp);
        if (constraint && constraint->IsActive())
        {
			// Recreate the constraint in the physics world
            constraint->CreateConstraint();
            LOG_DEBUG("[Application] Recreated constraint on '%s' for play mode", obj->GetName().c_str());
        }
    }

    // Recursivamente procesar hijos
    for (GameObject* child : obj->GetChildren())
    {
        RecreateConstraintsRecursive(child);
    }
}

void Application::MarkCollidersAsStandalone(GameObject* obj)
{
    if (!obj) return;

    // Get all colliders and mark them as not attached
    std::vector<Component*> colliders = obj->GetComponentsOfType(ComponentType::COLLIDER);
    for (Component* comp : colliders)
    {
        ComponentCollider* collider = static_cast<ComponentCollider*>(comp);
        if (collider)
        {
            collider->ForceStandaloneMode();
        }
    }

    // Recursively process children
    for (GameObject* child : obj->GetChildren())
    {
        MarkCollidersAsStandalone(child);
    }
}

void Application::DisableConstraintsRecursive(GameObject* obj)
{
    if (!obj) return;

    // Disable Constraints first
    std::vector<Component*> constraints = obj->GetComponentsOfType(ComponentType::CONSTRAINT);
    for (Component* comp : constraints)
    {
        if (comp && comp->IsActive())
        {
            comp->Disable();
        }
    }

    // Disable RigidBody
    for (GameObject* child : obj->GetChildren())
    {
        DisableConstraintsRecursive(child);
    }
}

void Application::DisableRigidBodiesRecursive(GameObject* obj)
{
    if (!obj) return;

    ComponentRigidBody* rb = static_cast<ComponentRigidBody*>(
        obj->GetComponent(ComponentType::RIGIDBODY)
        );
    if (rb && rb->IsActive())
    {
        rb->Disable();
    }

    // Then disable all colliders (this removes standalone colliders from physics world)
    for (GameObject* child : obj->GetChildren())
    {
        DisableRigidBodiesRecursive(child);
    }
}

void Application::DisableCollidersRecursive(GameObject* obj)
{
    if (!obj) return;

    std::vector<Component*> colliders = obj->GetComponentsOfType(ComponentType::COLLIDER);
    for (Component* comp : colliders)
    {
        ComponentCollider* collider = static_cast<ComponentCollider*>(comp);
        if (collider && collider->IsActive())
        {
            collider->Disable();
        }
    }

    // Recursively process children
    for (GameObject* child : obj->GetChildren())
    {
        DisableCollidersRecursive(child);
    }
}

void Application::CleanupPhysicsRecursive(GameObject* obj)
{
    if (!obj) return;

    ValidateConstraintsRecursive(obj);

    DisableConstraintsRecursive(obj);

    DisableRigidBodiesRecursive(obj);

    DisableCollidersRecursive(obj);
}

void Application::ValidateConstraintsRecursive(GameObject* obj)
{
    if (!obj) return;

    std::vector<Component*> constraints = obj->GetComponentsOfType(ComponentType::CONSTRAINT);

    std::vector<ComponentConstraint*> constraintsCopy;
    for (Component* comp : constraints)
    {
        constraintsCopy.push_back(static_cast<ComponentConstraint*>(comp));
    }

    for (ComponentConstraint* constraint : constraintsCopy)
    {
        if (constraint && !constraint->IsConstraintValid())
        {
            LOG_DEBUG("[Application] Removing invalid constraint from '%s'", obj->GetName().c_str());
            obj->RemoveComponent(constraint);
        }
    }

    for (GameObject* child : obj->GetChildren())
    {
        ValidateConstraintsRecursive(child);
    }
}