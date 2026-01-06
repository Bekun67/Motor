#pragma once

#include "Module.h"
#include <btBulletDynamicsCommon.h>
#include <memory>
#include <vector>

class GameObject;
class ComponentRigidBody;

class ModulePhysics : public Module
{
public:
    ModulePhysics();
    ~ModulePhysics();

    bool Awake() override;
    bool Start() override;
    bool PreUpdate() override;
    bool Update() override;
    bool PostUpdate() override;
    bool CleanUp() override;

    // Physics world management
    btDiscreteDynamicsWorld* GetDynamicsWorld() { return dynamicsWorld.get(); }

    // Configuration
    void SetGravity(const glm::vec3& gravity);
    glm::vec3 GetGravity() const;

    void SetSubsteps(int substeps) { maxSubSteps = substeps; }
    int GetSubsteps() const { return maxSubSteps; }

    void SetFixedTimeStep(float timeStep) { fixedTimeStep = timeStep; }
    float GetFixedTimeStep() const { return fixedTimeStep; }

    // Debug drawing
    void SetDebugDrawEnabled(bool enabled) { debugDrawEnabled = enabled; }
    bool IsDebugDrawEnabled() const { return debugDrawEnabled; }

    // Raycasting
    bool Raycast(const glm::vec3& from, const glm::vec3& to,
        btVector3& hitPoint, btVector3& hitNormal,
        GameObject** hitObject = nullptr);

private:
    // Bullet physics objects
    std::unique_ptr<btDefaultCollisionConfiguration> collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btBroadphaseInterface> broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
    std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;

    // Debug drawer
    class PhysicsDebugDrawer;
    std::unique_ptr<PhysicsDebugDrawer> debugDrawer;

    // Configuration
    glm::vec3 gravity;
    int maxSubSteps;
    float fixedTimeStep;
    bool debugDrawEnabled;

    // Simulation control
    bool isSimulationRunning;
    float accumulatedTime;
};