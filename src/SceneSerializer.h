#pragma once
#include <string>
#include <vector>
#include "GameObject.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class SceneSerializer
{
public:
	static bool SaveScene(const std::string& filepath, const std::vector<GameObject*>& gameObjects);
	static bool LoadScene(const std::string& filepath, std::vector<GameObject*>& gameObjects);

private:
	static json SerializeGameObject(const GameObject* go);
	static GameObject* DeserializeGameObject(const json& j, bool& success);
	static void ReconstructHierarchy(std::vector<GameObject*>& gameObjects);

	// Resource management during serialization
	static bool EnsureResourcesExist(const GameObject* go);
};