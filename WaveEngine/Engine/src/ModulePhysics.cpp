#include "ModulePhysics.h"
#include "Application.h"
#include "Time.h"
#include "Log.h"
#include "GameObject.h"
#include "ComponentRigidBody.h"
#include <glm/glm.hpp>

// Debug Drawer for Bullet Physics
class ModulePhysics::PhysicsDebugDrawer : public btIDebugDraw
{
public:
    PhysicsDebugDrawer() : debugMode(0) {}

    virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override
    {
        // TODO: Implement debug line drawing using your renderer
        // For now, we'll leave this empty
    }

    virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB,
        btScalar distance, int lifeTime, const btVector3& color) override
    {
        // TODO: Implement contact point drawing
    }

    virtual void reportErrorWarning(const char* warningString) override
    {
        LOG_CONSOLE("[Physics Warning] %s", warningString);
    }

    virtual void draw3dText(const btVector3& location, const char* textString) override
    {
        // TODO: Implement 3D text drawing
    }

    virtual void setDebugMode(int debugMode) override
    {
        this->debugMode = debugMode;
    }

    virtual int getDebugMode() const override
    {
        return debugMode;
    }

private:
    int debugMode;
};

ModulePhysics::ModulePhysics()
    : Module(),
    gravity(0.0f, -9.81f, 0.0f),
    maxSubSteps(10),
    fixedTimeStep(1.0f / 60.0f),
    debugDrawEnabled(false),
    isSimulationRunning(false),
    accumulatedTime(0.0f)
{
    name = "ModulePhysics";
}

ModulePhysics::~ModulePhysics()
{
}

bool ModulePhysics::Awake()
{
    LOG_DEBUG("[ModulePhysics] Awakening physics module");
    return true;
}

bool ModulePhysics::Start()
{
    LOG_CONSOLE("[ModulePhysics] Initializing Bullet Physics...");

    // Create collision configuration
    collisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();

    // Create collision dispatcher
    dispatcher = std::make_unique<btCollisionDispatcher>(collisionConfiguration.get());

    // Create broadphase
    broadphase = std::make_unique<btDbvtBroadphase>();

    // Create constraint solver
    solver = std::make_unique<btSequentialImpulseConstraintSolver>();

    // Create dynamics world
    dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        dispatcher.get(),
        broadphase.get(),
        solver.get(),
        collisionConfiguration.get()
    );

    // Set gravity
    dynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));

    // Create and set debug drawer
    debugDrawer = std::make_unique<PhysicsDebugDrawer>();
    dynamicsWorld->setDebugDrawer(debugDrawer.get());

    LOG_CONSOLE("[ModulePhysics] Bullet Physics initialized successfully");
    LOG_CONSOLE("  - Gravity: (%.2f, %.2f, %.2f)", gravity.x, gravity.y, gravity.z);
    LOG_CONSOLE("  - Fixed timestep: %.4f", fixedTimeStep);
    LOG_CONSOLE("  - Max substeps: %d", maxSubSteps);

    return true;
}

bool ModulePhysics::PreUpdate()
{
    return true;
}

bool ModulePhysics::Update()
{
    // Check if simulation should run based on play state
    Application& app = Application::GetInstance();
    Application::PlayState playState = app.GetPlayState();

    // Only simulate when playing
    isSimulationRunning = (playState == Application::PlayState::PLAYING);

    if (!isSimulationRunning)
    {
        accumulatedTime = 0.0f;
        return true;
    }

    // Get delta time from game time (respects time scale and pause)
    float deltaTime = app.time->GetDeltaTime();

    if (deltaTime > 0.0f)
    {
        // Step the simulation
        // Using variable timestep with substeps for stability
        dynamicsWorld->stepSimulation(deltaTime, maxSubSteps, fixedTimeStep);

        LOG_DEBUG("[ModulePhysics] Stepped simulation: dt=%.4f", deltaTime);
    }

    return true;
}

bool ModulePhysics::PostUpdate()
{
    // Debug drawing
    if (debugDrawEnabled && dynamicsWorld)
    {
        dynamicsWorld->debugDrawWorld();
    }

    return true;
}

bool ModulePhysics::CleanUp()
{
    LOG_CONSOLE("[ModulePhysics] Cleaning up physics module");

    // Remove all rigid bodies from the world before destroying it
    if (dynamicsWorld)
    {
        int numBodies = dynamicsWorld->getNumCollisionObjects();
        for (int i = numBodies - 1; i >= 0; i--)
        {
            btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);

            if (body)
            {
                dynamicsWorld->removeRigidBody(body);
            }
            else
            {
                dynamicsWorld->removeCollisionObject(obj);
            }
        }

        LOG_DEBUG("[ModulePhysics] Removed %d collision objects from world", numBodies);
    }

    // Clean up in reverse order of creation
    dynamicsWorld.reset();
    solver.reset();
    broadphase.reset();
    dispatcher.reset();
    collisionConfiguration.reset();
    debugDrawer.reset();

    LOG_CONSOLE("[ModulePhysics] Physics module cleaned up");
    return true;
}

void ModulePhysics::SetGravity(const glm::vec3& newGravity)
{
    gravity = newGravity;

    if (dynamicsWorld)
    {
        dynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));

        int numObjects = dynamicsWorld->getNumCollisionObjects();
        for (int i = 0; i < numObjects; i++)
        {
            btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);

            if (body && body->getInvMass() != 0.0f)
            {
                body->activate(true);
                body->clearForces();
            }
        }

        LOG_DEBUG("[ModulePhysics] Gravity set to (%.2f, %.2f, %.2f)",
            gravity.x, gravity.y, gravity.z);
    }
}

glm::vec3 ModulePhysics::GetGravity() const
{
    return gravity;
}

bool ModulePhysics::Raycast(const glm::vec3& from, const glm::vec3& to,
    btVector3& hitPoint, btVector3& hitNormal,
    GameObject** hitObject)
{
    if (!dynamicsWorld)
        return false;

    btVector3 btFrom(from.x, from.y, from.z);
    btVector3 btTo(to.x, to.y, to.z);

    btCollisionWorld::ClosestRayResultCallback rayCallback(btFrom, btTo);

    dynamicsWorld->rayTest(btFrom, btTo, rayCallback);

    if (rayCallback.hasHit())
    {
        hitPoint = rayCallback.m_hitPointWorld;
        hitNormal = rayCallback.m_hitNormalWorld;

        // Try to get GameObject from user pointer
        if (hitObject && rayCallback.m_collisionObject)
        {
            void* userPtr = rayCallback.m_collisionObject->getUserPointer();
            if (userPtr)
            {
                *hitObject = static_cast<GameObject*>(userPtr);
            }
        }

        return true;
    }

    return false;
}