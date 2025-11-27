#include "SceneSerializer.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "ModuleEditor.h"
#include "FileSystemManager.h"
#include "TextureImporter.h"
#include "ResourceManager.h"
#include "MeshImporter.h"
#include <glad/glad.h>
#include <fstream>
#include <iostream>

bool SceneSerializer::SaveScene(const std::string& filepath, const std::vector<GameObject*>& gameObjects)
{
	LOG("Saving scene to: " + filepath);

	json sceneJson;
	sceneJson["GameObjects"] = json::array();

	for (const GameObject* go : gameObjects)
	{
		if (go != nullptr)
		{
			// Ensure resources exist before saving
			if (!EnsureResourcesExist(go))
			{
				LOG_WARNING("Some resources for GameObject '" + go->name + "' could not be saved");
			}

			sceneJson["GameObjects"].push_back(SerializeGameObject(go));
		}
	}

	std::ofstream file(filepath);
	if (!file.is_open())
	{
		LOG_ERROR("Failed to save scene to: " + filepath);
		return false;
	}

	file << sceneJson.dump(4); // Pretty print with 4 spaces
	file.close();

	LOG("Scene saved successfully to: " + filepath);
	return true;
}

bool SceneSerializer::LoadScene(const std::string& filepath, std::vector<GameObject*>& gameObjects)
{
	LOG("Loading scene from: " + filepath);

	std::ifstream file(filepath);
	if (!file.is_open())
	{
		LOG_ERROR("Failed to load scene from: " + filepath);
		return false;
	}

	json sceneJson;
	try
	{
		file >> sceneJson;
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Failed to parse scene JSON: " + std::string(e.what()));
		file.close();
		return false;
	}
	file.close();

	// Safe cleanup: Find roots only
	std::vector<GameObject*> rootsToDelete;
	for (GameObject* go : gameObjects)
	{
		if (go && go->parent == nullptr)
		{
			rootsToDelete.push_back(go);
		}
	}

	// Clear list first
	gameObjects.clear();

	// Delete roots
	for (GameObject* root : rootsToDelete)
	{
		delete root;
	}

	// Deserialize new scene
	if (sceneJson.contains("GameObjects") && sceneJson["GameObjects"].is_array())
	{
		for (const auto& goJson : sceneJson["GameObjects"])
		{
			bool success = false;
			GameObject* go = DeserializeGameObject(goJson, success);

			if (go != nullptr && success)
			{
				gameObjects.push_back(go);
			}
			else if (go != nullptr)
			{
				std::string name = go->name;
				delete go;
				LOG_ERROR("Failed to load GameObject: " + name + " - skipping");
			}
		}
	}

	// Reconstruct hierarchy
	ReconstructHierarchy(gameObjects);

	LOG("Scene loaded successfully from: " + filepath);
	LOG("Loaded " + std::to_string(gameObjects.size()) + " GameObjects");
	return true;
}

json SceneSerializer::SerializeGameObject(const GameObject* go)
{
	json j;

	j["UUID"] = go->GetUUID().ToString();
	j["ParentUUID"] = go->GetParentUUID().ToString();
	j["Name"] = go->name;
	j["Active"] = go->active;
	j["MeshPath"] = go->meshPath;

	// Mark if this is an empty GameObject
	j["IsEmpty"] = go->IsEmpty();

	// Serialize mesh index in FBX (not engine index)
	if (go->mesh && go->mesh->meshIndex >= 0 && !go->meshPath.empty())
	{
		// Store the mesh index within its FBX file
		j["MeshIndexInFBX"] = go->meshIndexInFBX;
	}
	else
	{
		// Empty GameObject
		j["MeshIndexInFBX"] = -1;
	}

	// Serialize Transform
	if (go->transform)
	{
		PropertyMap transformProps = go->transform->Serialize();
		json transformJson;
		for (const auto& [key, value] : transformProps)
		{
			if (std::holds_alternative<float>(value))
				transformJson[key] = std::get<float>(value);
			else if (std::holds_alternative<double>(value))
				transformJson[key] = std::get<double>(value);
			else if (std::holds_alternative<int>(value))
				transformJson[key] = std::get<int>(value);
			else if (std::holds_alternative<bool>(value))
				transformJson[key] = std::get<bool>(value);
			else if (std::holds_alternative<std::string>(value))
				transformJson[key] = std::get<std::string>(value);
		}
		j["Transform"] = transformJson;
	}

	// Serialize Mesh
	if (go->mesh)
	{
		PropertyMap meshProps = go->mesh->Serialize();
		json meshJson;
		for (const auto& [key, value] : meshProps)
		{
			if (std::holds_alternative<int>(value))
				meshJson[key] = std::get<int>(value);
			else if (std::holds_alternative<bool>(value))
				meshJson[key] = std::get<bool>(value);
		}
		j["Mesh"] = meshJson;
	}

	// Serialize Texture
	if (go->texture && !go->IsEmpty())
	{
		PropertyMap textureProps = go->texture->Serialize();
		json textureJson;
		for (const auto& [key, value] : textureProps)
		{
			if (std::holds_alternative<std::string>(value))
				textureJson[key] = std::get<std::string>(value);
			else if (std::holds_alternative<bool>(value))
				textureJson[key] = std::get<bool>(value);
		}
		j["Texture"] = textureJson;
	}

	return j;
}

GameObject* SceneSerializer::DeserializeGameObject(const json& j, bool& success)
{
	success = false;
	GameObject* go = new GameObject();

	// Deserialize UUID
	if (j.contains("UUID"))
	{
		std::string uuidStr = j["UUID"];
		go->SetUUID(UUID::FromString(uuidStr));
	}

	if (j.contains("ParentUUID"))
	{
		std::string parentUuidStr = j["ParentUUID"];
		go->SetParentUUID(UUID::FromString(parentUuidStr));
	}

	if (j.contains("Name"))
		go->name = j["Name"];

	if (j.contains("Active"))
		go->active = j["Active"];

	if (j.contains("MeshPath"))
		go->meshPath = j["MeshPath"];

	// Deserialize Transform
	if (j.contains("Transform") && go->transform)
	{
		try
		{
			PropertyMap transformProps;
			for (auto& [key, value] : j["Transform"].items())
			{
				if (value.is_number_float())
					transformProps[key] = value.get<float>();
				else if (value.is_number_integer())
					transformProps[key] = value.get<int>();
				else if (value.is_boolean())
					transformProps[key] = value.get<bool>();
				else if (value.is_string())
					transformProps[key] = value.get<std::string>();
			}
			go->transform->Deserialize(transformProps);
		}
		catch (const std::exception& e)
		{
			LOG_ERROR("Error deserializing Transform for " + go->name + ": " + std::string(e.what()));
			delete go;
			return nullptr;
		}
	}

	// Deserialize and load Mesh
	if (j.contains("Mesh") && go->mesh)
	{
		try
		{
			// Check if this is an empty GameObject (no mesh)
			bool isEmpty = go->meshPath.empty() || go->meshPath == "";

			if (!isEmpty)
			{
				int meshIndexInFBX = 0;
				if (j.contains("MeshIndexInFBX"))
				{
					meshIndexInFBX = j["MeshIndexInFBX"];
					go->meshIndexInFBX = meshIndexInFBX;
				}

				// Try to load mesh using ResourceManager
				int engineMeshIndex = -1;
				if (ResourceManager::EnsureMeshExists(go->meshPath, meshIndexInFBX, engineMeshIndex))
				{
					go->mesh->meshIndex = engineMeshIndex;
					LOG("Mesh loaded for GameObject: " + go->name + " at engine index: " + std::to_string(engineMeshIndex));
				}
				else
				{
					LOG_ERROR("Failed to load mesh for GameObject: " + go->name + " from path: " + go->meshPath);
					// Don't fail completely, just make it an empty GameObject
					go->mesh->meshIndex = -1;
					go->meshPath = "";
					LOG_WARNING("Converted " + go->name + " to empty GameObject due to missing mesh");
				}
			}
			else
			{
				// This is an empty GameObject
				go->mesh->meshIndex = -1;
				LOG("Loaded empty GameObject: " + go->name);
			}

			// Deserialize other mesh properties
			PropertyMap meshProps;
			for (auto& [key, value] : j["Mesh"].items())
			{
				if (value.is_number_integer())
					meshProps[key] = value.get<int>();
				else if (value.is_boolean())
					meshProps[key] = value.get<bool>();
			}

			// Don't override meshIndex if we already set it
			meshProps.erase("meshIndex");

			go->mesh->Deserialize(meshProps);
		}
		catch (const std::exception& e)
		{
			LOG_ERROR("Error deserializing Mesh for " + go->name + ": " + std::string(e.what()));
			// Don't fail, just make it empty
			go->mesh->meshIndex = -1;
			go->meshPath = "";
			LOG_WARNING("Converted " + go->name + " to empty GameObject due to deserialization error");
		}
	}

	// Deserialize Texture
	if (j.contains("Texture") && go->texture)
	{
		try
		{
			PropertyMap textureProps;
			for (auto& [key, value] : j["Texture"].items())
			{
				if (value.is_string())
					textureProps[key] = value.get<std::string>();
				else if (value.is_boolean())
					textureProps[key] = value.get<bool>();
			}

			// Ensure texture resource exists before deserializing
			if (textureProps.count("texturePath"))
			{
				std::string texPath = std::get<std::string>(textureProps["texturePath"]);
				if (!texPath.empty() && texPath != "checkerboard")
				{
					ResourceManager::EnsureTextureExists(texPath);
				}
			}

			go->texture->Deserialize(textureProps);
		}
		catch (const std::exception& e)
		{
			LOG_WARNING("Error deserializing Texture for " + go->name + ": " + std::string(e.what()));
			// Don't fail the whole GameObject for texture issues, just use checkerboard
			go->texture->CreateCheckerboardTexture();
		}
	}

	success = true;
	return go;
}

void SceneSerializer::ReconstructHierarchy(std::vector<GameObject*>& gameObjects)
{
	// Create a map of UUID to GameObject for quick lookup
	std::map<uint32_t, GameObject*> uuidMap;
	for (GameObject* go : gameObjects)
	{
		uuidMap[(uint32_t)go->GetUUID()] = go;
	}

	// Reconstruct parent-child relationships
	for (GameObject* go : gameObjects)
	{
		uint32_t parentUUID = (uint32_t)go->GetParentUUID();
		if (parentUUID != 0 && uuidMap.count(parentUUID))
		{
			GameObject* parentGo = uuidMap[parentUUID];
			go->parent = parentGo;
			parentGo->children.push_back(go);
		}
		else
		{
			go->parent = nullptr;
		}
	}
}

bool SceneSerializer::EnsureResourcesExist(const GameObject* go)
{
	bool success = true;

	// Skip resource checking for empty GameObjects
	if (go->IsEmpty())
	{
		return true;
	}

	// Ensure mesh resource exists
	if (go->mesh && go->mesh->meshIndex >= 0 && !go->meshPath.empty())
	{
		int meshIndexInFBX = go->meshIndexInFBX;
		int dummyIndex;
		if (!ResourceManager::EnsureMeshExists(go->meshPath, meshIndexInFBX, dummyIndex))
		{
			LOG_WARNING("Failed to ensure mesh exists for: " + go->name);
			success = false;
		}
	}

	// Ensure texture resource exists
	if (go->texture && !go->texture->texturePath.empty() && go->texture->texturePath != "checkerboard")
	{
		if (!ResourceManager::EnsureTextureExists(go->texture->texturePath))
		{
			LOG_WARNING("Failed to ensure texture exists for: " + go->name);
		}
	}

	return success;
}