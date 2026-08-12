#pragma once 

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <string>


// NOTE: Sphere/Plane/Cylinder/Prism are new "basic shape" types for the
// drag-and-drop workflow. They currently share the Cube's mesh as a
// placeholder (see Triangle::Draw) until dedicated geometry is added.
enum class ObjectType { Cube, Sphere, Plane, Cylinder, Prism, Light, Ground };

// Which loaded texture (if any) a basic shape should be rendered with.
enum class TextureSlot { None, Container, Grass };

struct GameObject {
	ObjectType type;
	glm::vec3 position;
	glm::vec3 scale = glm::vec3(1.0f);
	std::string name;
	bool rotates = true;
	glm::vec3 rotation = glm::vec3(0.0f);

	// ADD THIS: Persistent matrix initialized to Identity
	glm::mat4 transformMatrix = glm::mat4(1.0f);

	// NEW: Per-object configuration set from the Objects > Shapes panel
	// (or defaults, for models loaded later from Blender/etc).
	glm::vec3 baseColor = glm::vec3(1.0f, 0.5f, 0.31f);
	TextureSlot textureSlot = TextureSlot::None;
	bool isBasicShape = true; // false is reserved for imported models later
};

inline std::string ToString(ObjectType type) {
	switch (type) {
	case ObjectType::Cube:     return "Cube";
	case ObjectType::Sphere:   return "Sphere";
	case ObjectType::Plane:    return "Plane";
	case ObjectType::Cylinder: return "Cylinder";
	case ObjectType::Prism:    return "Prism";
	case ObjectType::Light:    return "Light";
	case ObjectType::Ground:   return "Ground";
	}
	return "Unknown";
}