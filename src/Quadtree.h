#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Ray.h"

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

private:
    void Subdivide();

    bool Contains(GameObject* object) const;

    int GetChildIndex(GameObject* object) const;

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

    void GetAllObjects(std::vector<GameObject*>& results) const;

    void DebugDraw() const;

    const AABB& GetBoundary() const;
    bool IsEmpty() const { return root == nullptr; }

private:
    QuadtreeNode* root;
    int maxObjects;
    int maxLevels;
};