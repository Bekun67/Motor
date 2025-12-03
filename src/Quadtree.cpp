#include "Quadtree.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include "LoadFBX.h"
#include "Application.h"
#include "OpenGL.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

QuadtreeNode::QuadtreeNode(const AABB& boundary, int maxObjects, int maxLevels, int level)
    : boundary(boundary), maxObjects(maxObjects), maxLevels(maxLevels), level(level)
{
    for (int i = 0; i < 8; ++i)
    {
        children[i] = nullptr;
    }
}

QuadtreeNode::~QuadtreeNode()
{
    Clear();
}

void QuadtreeNode::Clear()
{
    objects.clear();

    for (int i = 0; i < 8; ++i)
    {
        if (children[i] != nullptr)
        {
            delete children[i];
            children[i] = nullptr;
        }
    }
}

bool QuadtreeNode::Insert(GameObject* object)
{
    if (object == nullptr || object->mesh == nullptr) return false;

    //if it doesnt interesct we dont add it to the node
    if (!Intersects(object)) return false;

    if (IsLeaf())
    {
        //if there is space for a new game object in the node we add it
        if (objects.size() < (size_t)maxObjects || level >= maxLevels)
        {
            objects.push_back(object);
            return true;
        }
        else
        {
            //if there's no space (>8) we subdivide
            Subdivide();

            std::vector<GameObject*> tempObjects = objects;
            objects.clear();

            for (GameObject* obj : tempObjects)
            {
                bool inserted = false;

                for (int i = 0; i < 8; ++i)
                {
                    if (children[i]->CanContainCompletely(obj))
                    {
                        children[i]->Insert(obj);
                        inserted = true;
                        break;
                    }
                }

                if (!inserted)
                {
                    objects.push_back(obj);
                }
            }

            bool inserted = false;
            for (int i = 0; i < 8; ++i)
            {
                if (children[i]->CanContainCompletely(object))
                {
                    children[i]->Insert(object);
                    inserted = true;
                    break;
                }
            }

            if (!inserted)
            {
                objects.push_back(object);
            }

            return true;
        }
    }
    else
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i]->CanContainCompletely(object))
            {
                return children[i]->Insert(object);
            }
        }

        objects.push_back(object);
        return true;
    }
}

bool QuadtreeNode::Remove(GameObject* object)
{
    if (object == nullptr)
        return false;

    auto it = std::find(objects.begin(), objects.end(), object);
    if (it != objects.end())
    {
        objects.erase(it);
        return true;
    }

    if (!IsLeaf())
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i]->Remove(object))
            {
                return true;
            }
        }
    }

    return false;
}

void QuadtreeNode::Subdivide()
{
    if (!IsLeaf()) return;

    //dividing in 8
    glm::vec3 min = boundary.min;
    glm::vec3 max = boundary.max;
    glm::vec3 center = boundary.GetCenter();

    children[0] = new QuadtreeNode(
        AABB(glm::vec3(min.x, center.y, center.z), glm::vec3(center.x, max.y, max.z)),
        maxObjects, maxLevels, level + 1);

    children[1] = new QuadtreeNode(
        AABB(glm::vec3(center.x, center.y, center.z), glm::vec3(max.x, max.y, max.z)),
        maxObjects, maxLevels, level + 1);

    children[2] = new QuadtreeNode(
        AABB(glm::vec3(min.x, center.y, min.z), glm::vec3(center.x, max.y, center.z)),
        maxObjects, maxLevels, level + 1);

    children[3] = new QuadtreeNode(
        AABB(glm::vec3(center.x, center.y, min.z), glm::vec3(max.x, max.y, center.z)),
        maxObjects, maxLevels, level + 1);

    children[4] = new QuadtreeNode(
        AABB(glm::vec3(min.x, min.y, center.z), glm::vec3(center.x, center.y, max.z)),
        maxObjects, maxLevels, level + 1);

    children[5] = new QuadtreeNode(
        AABB(glm::vec3(center.x, min.y, center.z), glm::vec3(max.x, center.y, max.z)),
        maxObjects, maxLevels, level + 1);

    children[6] = new QuadtreeNode(
        AABB(glm::vec3(min.x, min.y, min.z), glm::vec3(center.x, center.y, center.z)),
        maxObjects, maxLevels, level + 1);

    children[7] = new QuadtreeNode(
        AABB(glm::vec3(center.x, min.y, min.z), glm::vec3(max.x, center.y, center.z)),
        maxObjects, maxLevels, level + 1);
}

bool QuadtreeNode::CanContainCompletely(GameObject* object) const
{
    //we check if all the object's vertices are inside the node
    if (object == nullptr || object->mesh == nullptr) return false;

    WorldAABB worldAABB = object->mesh->GetWorldAABB();

    bool xContained = worldAABB.min.x >= boundary.min.x && worldAABB.max.x <= boundary.max.x;
    bool yContained = worldAABB.min.y >= boundary.min.y && worldAABB.max.y <= boundary.max.y;
    bool zContained = worldAABB.min.z >= boundary.min.z && worldAABB.max.z <= boundary.max.z;

    return xContained && yContained && zContained;
}

bool QuadtreeNode::Intersects(GameObject* object) const
{
    //we check if the game object intersects with the node
    if (object == nullptr || object->mesh == nullptr)
        return false;

    WorldAABB worldAABB = object->mesh->GetWorldAABB();

    bool xOverlap = worldAABB.min.x <= boundary.max.x && worldAABB.max.x >= boundary.min.x;
    bool yOverlap = worldAABB.min.y <= boundary.max.y && worldAABB.max.y >= boundary.min.y;
    bool zOverlap = worldAABB.min.z <= boundary.max.z && worldAABB.max.z >= boundary.min.z;

    return xOverlap && yOverlap && zOverlap;
}

void QuadtreeNode::Intersect(std::vector<GameObject*>& results, const AABB& area) const
{
    bool xOverlap = boundary.min.x <= area.max.x && boundary.max.x >= area.min.x;
    bool yOverlap = boundary.min.y <= area.max.y && boundary.max.y >= area.min.y;
    bool zOverlap = boundary.min.z <= area.max.z && boundary.max.z >= area.min.z;

    if (!xOverlap || !yOverlap || !zOverlap) return;

    for (GameObject* obj : objects)
    {
        if (obj != nullptr)
        {
            results.push_back(obj);
        }
    }

    if (!IsLeaf())
    {
        for (int i = 0; i < 8; ++i)
        {
            children[i]->Intersect(results, area);
        }
    }
}

void QuadtreeNode::Intersect(std::vector<GameObject*>& results, const Ray& ray) const
{
    float tMin, tMax;
    if (!boundary.IntersectRay(ray, tMin, tMax))
        return;

    for (GameObject* obj : objects)
    {
        if (obj != nullptr)
        {
            results.push_back(obj);
        }
    }

    if (!IsLeaf())
    {
        for (int i = 0; i < 8; ++i)
        {
            children[i]->Intersect(results, ray);
        }
    }
}

void QuadtreeNode::GetAllObjects(std::vector<GameObject*>& results) const
{
    for (GameObject* obj : objects)
    {
        if (obj != nullptr)
        {
            results.push_back(obj);
        }
    }

    if (!IsLeaf())
    {
        for (int i = 0; i < 8; ++i)
        {
            children[i]->GetAllObjects(results);
        }
    }
}

void QuadtreeNode::DebugDraw() const
{
    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (!opengl)
        return;

    std::vector<float> lineData;

    glm::vec3 min = boundary.min;
    glm::vec3 max = boundary.max;

    float y = 0.0f;

    lineData.insert(lineData.end(), { min.x, min.y, min.z });
    lineData.insert(lineData.end(), { max.x, min.y, min.z });

    lineData.insert(lineData.end(), { max.x, min.y, min.z });
    lineData.insert(lineData.end(), { max.x, min.y, max.z });

    lineData.insert(lineData.end(), { max.x, min.y, max.z });
    lineData.insert(lineData.end(), { min.x, min.y, max.z });

    lineData.insert(lineData.end(), { min.x, min.y, max.z });
    lineData.insert(lineData.end(), { min.x, min.y, min.z });

    lineData.insert(lineData.end(), { min.x, max.y, min.z });
    lineData.insert(lineData.end(), { max.x, max.y, min.z });

    lineData.insert(lineData.end(), { max.x, max.y, min.z });
    lineData.insert(lineData.end(), { max.x, max.y, max.z });

    lineData.insert(lineData.end(), { max.x, max.y, max.z });
    lineData.insert(lineData.end(), { min.x, max.y, max.z });

    lineData.insert(lineData.end(), { min.x, max.y, max.z });
    lineData.insert(lineData.end(), { min.x, max.y, min.z });

    lineData.insert(lineData.end(), { min.x, min.y, min.z });
    lineData.insert(lineData.end(), { min.x, max.y, min.z });

    lineData.insert(lineData.end(), { max.x, min.y, min.z });
    lineData.insert(lineData.end(), { max.x, max.y, min.z });

    lineData.insert(lineData.end(), { max.x, min.y, max.z });
    lineData.insert(lineData.end(), { max.x, max.y, max.z });

    lineData.insert(lineData.end(), { min.x, min.y, max.z });
    lineData.insert(lineData.end(), { min.x, max.y, max.z });

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int shader = opengl->normalShaderProgram;
    glUseProgram(shader);

    glm::mat4 identity = glm::mat4(1.0f);
    glm::mat4 view = opengl->camera.GetViewMatrix();
    glm::mat4 projection = opengl->camera.GetProjectionMatrix();

    glUniformMatrix4fv(glGetUniformLocation(shader, "model_matrix"), 1, GL_FALSE, glm::value_ptr(identity));
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    float colorIntensity = 1.0f - (level * 0.15f);
    glUniform3f(glGetUniformLocation(shader, "lineColor"), 0.0f, colorIntensity, colorIntensity);

    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, lineData.size() / 3);
    glLineWidth(1.0f);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    if (!IsLeaf())
    {
        for (int i = 0; i < 8; ++i)
        {
            children[i]->DebugDraw();
        }
    }
}

Quadtree::Quadtree()
    : root(nullptr), maxObjects(4), maxLevels(5)
{
}

Quadtree::Quadtree(const AABB& boundary, int maxObjects, int maxLevels)
    : root(nullptr), maxObjects(maxObjects), maxLevels(maxLevels)
{
    Create(boundary, maxObjects, maxLevels);
}

Quadtree::~Quadtree()
{
    Clear();
}

void Quadtree::Create(const AABB& boundary, int maxObjects, int maxLevels)
{
    Clear();
    this->maxObjects = maxObjects;
    this->maxLevels = maxLevels;
    root = new QuadtreeNode(boundary, maxObjects, maxLevels, 0);
}

void Quadtree::Clear()
{
    if (root != nullptr)
    {
        delete root;
        root = nullptr;
    }
}

bool Quadtree::Insert(GameObject* object)
{
    if (root == nullptr)
        return false;

    return root->Insert(object);
}

bool Quadtree::Remove(GameObject* object)
{
    if (root == nullptr)
        return false;

    return root->Remove(object);
}

void Quadtree::Intersect(std::vector<GameObject*>& results, const AABB& area) const
{
    if (root != nullptr)
    {
        root->Intersect(results, area);
    }
}

void Quadtree::Intersect(std::vector<GameObject*>& results, const Ray& ray) const
{
    if (root != nullptr)
    {
        root->Intersect(results, ray);
    }
}

void Quadtree::GetAllObjects(std::vector<GameObject*>& results) const
{
    if (root != nullptr)
    {
        root->GetAllObjects(results);
    }
}

void Quadtree::DebugDraw() const
{
    if (root != nullptr)
    {
        root->DebugDraw();
    }
}

const AABB& Quadtree::GetBoundary() const
{
    static AABB emptyAABB;
    if (root != nullptr)
    {
        return root->GetBoundary();
    }
    return emptyAABB;
}

void QuadtreeNode::CollectIntersections(std::vector<GameObject*>& results, const Ray& ray) const
{
    float tMin, tMax;
    if (!boundary.IntersectRay(ray, tMin, tMax))
    {
        //if the ray doesn't touch this node we discard it
        return;
    }

    for (GameObject* obj : objects)
    {
        if (obj != nullptr && obj->mesh != nullptr && obj->mesh->meshIndex >= 0)
        {
            if (std::find(results.begin(), results.end(), obj) == results.end())
            {
                results.push_back(obj);
            }
        }
    }

    if (!IsLeaf())
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i] != nullptr)
            {
                children[i]->CollectIntersections(results, ray);
            }
        }
    }
}

void Quadtree::CollectIntersections(std::vector<GameObject*>& results, const Ray& ray) const
{
    if (root != nullptr)
    {
        root->CollectIntersections(results, ray);
    }
}