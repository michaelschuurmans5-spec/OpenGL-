#pragma once 

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <string>

// NOTE: Sphere/Plane/Cylinder/Prism are new "basic shape" types for the
// drag-and-drop workflow. They currently share the Cube's mesh as a
// placeholder (see Triangle::Draw) until dedicated geometry is added.
// Terrain is the Level Designer > Terrain Generator's procedural mesh -
// see Triangle::PreviewTerrain / CommitTerrain / DeleteTerrain.
enum class ObjectType { Cube, Sphere, Plane, Cylinder, Prism, Light, Ground, Terrain, Prop };

// Which loaded texture (if any) a basic shape should be rendered with.
enum class TextureSlot { None, Container, Grass };

// Every GameObject gets a unique, never-reused runtime ID. The Viewport
// Manager (outliner) uses it as a stable handle so renaming an object never
// breaks the reference, and so 10 identical cubes stay tellable apart.
inline int NextObjectId() {
	static int s_nextObjectId = 1;
	return s_nextObjectId++;
}

struct GameObject {
	// Unique per-instance ID assigned at construction.
	int id = NextObjectId();

	ObjectType type;
	glm::vec3 position;
	glm::vec3 scale = glm::vec3(1.0f);
	std::string name;
	bool rotates = true;
	glm::vec3 rotation = glm::vec3(0.0f);

	// Viewport Manager state
	bool visible = true; // eye icon: hidden objects are skipped when rendering
	bool locked = false;  // lock icon: while any object is locked it is the only selectable object

	// Persistent matrix initialized to Identity
	glm::mat4 transformMatrix = glm::mat4(1.0f);

	// Per-object configuration set from the Objects > Shapes panel
	// (or defaults, for models loaded later from Blender/etc).
	glm::vec3 baseColor = glm::vec3(1.0f, 0.5f, 0.31f);
	TextureSlot textureSlot = TextureSlot::None;
	bool isBasicShape = true; // false is reserved for imported models later

	// Imported model information
	// Empty for basic shapes; contains the model key/path for Prop objects.
	std::string modelName;

	unsigned int textureOverride = 0; // 0 = use the model's own textures
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
	case ObjectType::Terrain:  return "Terrain";
	case ObjectType::Prop:     return "Prop";
	}
	return "Unknown";
}