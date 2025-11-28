#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Ray.h"
#include "Structures.h"

class GameObject;

class QuadtreeNode
{
public:
    QuadtreeNode(const AABB& boundary, int maxObjects = 4, int maxLevels = 5, int level = 0);
    ~QuadtreeNode();

    bool Insert(GameObject* object);
    bool Remove(GameObject* object);
    void Clear();

    void Intersect(std::vector<GameObject*>& results, const AABB& area) const;
    void Intersect(std::vector<GameObject*>& results, const Ray& ray) const;

    void GetAllObjects(std::vector<GameObject*>& results) const;
    void DebugDraw() const;

    const AABB& GetBoundary() const { return boundary; }
    int GetLevel() const { return level; }
    bool IsLeaf() const { return children[0] == nullptr; }

    //frustum template
    template<typename TYPE>
    inline void CollectIntersections(std::vector<GameObject*>& results, const TYPE& primitive) const
    {
        //if the node doesnt interesct we discard it
        if (!primitive.Intersects(boundary))
        {
            return;
        }

        //if the node interescts we add all objects in the node as candidates
        for (GameObject* obj : objects)
        {
            if (obj != nullptr && obj->mesh != nullptr)
            {
                results.push_back(obj);
            }
        }

        //try all children
        if (!IsLeaf())
        {
            for (int i = 0; i < 4; ++i)
            {
                if (children[i] != nullptr)
                {
                    children[i]->CollectIntersections(results, primitive);
                }
            }
        }
    }

private:
    void Subdivide();

    bool CanContainCompletely(GameObject* object) const;

    bool Intersects(GameObject* object) const;

    AABB boundary;
    std::vector<GameObject*> objects;
    QuadtreeNode* children[4];

    int maxObjects;
    int maxLevels;
    int level;
};

class Quadtree
{
public:
    Quadtree();
    Quadtree(const AABB& boundary, int maxObjects = 4, int maxLevels = 5);
    ~Quadtree();

    void Create(const AABB& boundary, int maxObjects = 4, int maxLevels = 5);
    void Clear();

    bool Insert(GameObject* object);
    bool Remove(GameObject* object);

    void Intersect(std::vector<GameObject*>& results, const AABB& area) const;
    void Intersect(std::vector<GameObject*>& results, const Ray& ray) const;

    template<typename TYPE>
    inline void CollectIntersections(std::vector<GameObject*>& results, const TYPE& primitive) const
    {
        if (root != nullptr)
        {
            root->CollectIntersections(results, primitive);
        }
    }

    void GetAllObjects(std::vector<GameObject*>& results) const;
    void DebugDraw() const;

    const AABB& GetBoundary() const;
    bool IsEmpty() const { return root == nullptr; }

private:
    QuadtreeNode* root;
    int maxObjects;
    int maxLevels;
};