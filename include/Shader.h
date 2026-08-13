#pragma once 


#include <glad/glad.h> 

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Shader {

public:
	// Program ID allocated by OpenGL
	unsigned int ID;

	// Constructor reads and builds the shader automatically 
	Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);


	// Destructor to clean up GPU memory
	~Shader();

	// Active shader program
	void use() const;

	// uniform setters helper function 
	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, int value) const;
	void setFloat(const std::string& name, float value) const;
	void setVec4(const std::string& name, float x, float y, float z, float w) const;
	void setVec3(const std::string& name, const glm::vec3& value) const;
	void setMat4(const std::string& name, const glm::mat4& mat) const;
};