#include "ComponentTransform.h"
#include "GameObject.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/scene.h>

ComponentTransform::ComponentTransform(GameObject* gameObject)
	: Component(gameObject, ComponentType::TRANSFORM)
{
	//predeterminated values
	scaling = aiVector3D(1.0f, 1.0f, 1.0f);
	translation = aiVector3D(0.0f, 0.0f, 0.0f);
	rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
	radius = g_ModelRadius;

	lastTranslation = translation;
	lastRotation = rotation;
	lastScaling = scaling;
}

ComponentTransform::~ComponentTransform()
{

}

void ComponentTransform::Update()
{
}

bool ComponentTransform::HasChanged()
{
	bool changed = false;
	const float epsilon = 0.0001f; 

	//check translation change
	if (std::abs(translation.x - lastTranslation.x) > epsilon ||
		std::abs(translation.y - lastTranslation.y) > epsilon ||
		std::abs(translation.z - lastTranslation.z) > epsilon)
	{
		changed = true;
	}

	//check rotation change
	if (std::abs(rotation.w - lastRotation.w) > epsilon ||
		std::abs(rotation.x - lastRotation.x) > epsilon ||
		std::abs(rotation.y - lastRotation.y) > epsilon ||
		std::abs(rotation.z - lastRotation.z) > epsilon)
	{
		changed = true;
	}

	//check scale change
	if (std::abs(scaling.x - lastScaling.x) > epsilon ||
		std::abs(scaling.y - lastScaling.y) > epsilon ||
		std::abs(scaling.z - lastScaling.z) > epsilon)
	{
		changed = true;
	}

	//if changed
	if (changed)
	{
		lastTranslation = translation;
		lastRotation = rotation;
		lastScaling = scaling;

		//mark for quadtree update
		if (gameObject && gameObject->isStatic)
		{
			gameObject->needsQuadtreeUpdate = true;
		}
	}

	return changed;
}

bool ComponentTransform::Decompose(const glm::mat4& transform, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale)
{
	glm::mat4 localMatrix(transform);

	translation = glm::vec3(localMatrix[3]);

	glm::vec3 row[3];

	for (int i = 0; i < 3; i++)
	{
		row[i] = glm::vec3(localMatrix[i]);
	}

	scale.x = glm::length(row[0]);
	scale.y = glm::length(row[1]);
	scale.z = glm::length(row[2]);

	if (scale.x != 0) row[0] /= scale.x;
	if (scale.y != 0) row[1] /= scale.y;
	if (scale.z != 0) row[2] /= scale.z;

	glm::mat3 rotationMatrix(row[0], row[1], row[2]);

	rotation = glm::quat_cast(rotationMatrix);

	return true;
}

PropertyMap ComponentTransform::Serialize() const
{
	PropertyMap props;

	props["translation_x"] = (float)translation.x;
	props["translation_y"] = (float)translation.y;
	props["translation_z"] = (float)translation.z;

	props["rotation_w"] = (float)rotation.w;
	props["rotation_x"] = (float)rotation.x;
	props["rotation_y"] = (float)rotation.y;
	props["rotation_z"] = (float)rotation.z;

	props["scale_x"] = (float)scaling.x;
	props["scale_y"] = (float)scaling.y;
	props["scale_z"] = (float)scaling.z;

	props["radius"] = radius;

	return props;
}

void ComponentTransform::Deserialize(const PropertyMap& props)
{
	if (props.count("translation_x")) translation.x = std::get<float>(props.at("translation_x"));
	if (props.count("translation_y")) translation.y = std::get<float>(props.at("translation_y"));
	if (props.count("translation_z")) translation.z = std::get<float>(props.at("translation_z"));

	if (props.count("rotation_w")) rotation.w = std::get<float>(props.at("rotation_w"));
	if (props.count("rotation_x")) rotation.x = std::get<float>(props.at("rotation_x"));
	if (props.count("rotation_y")) rotation.y = std::get<float>(props.at("rotation_y"));
	if (props.count("rotation_z")) rotation.z = std::get<float>(props.at("rotation_z"));

	if (props.count("scale_x")) scaling.x = std::get<float>(props.at("scale_x"));
	if (props.count("scale_y")) scaling.y = std::get<float>(props.at("scale_y"));
	if (props.count("scale_z")) scaling.z = std::get<float>(props.at("scale_z"));

	if (props.count("radius")) radius = std::get<float>(props.at("radius"));

	lastTranslation = translation;
	lastRotation = rotation;
	lastScaling = scaling;
}