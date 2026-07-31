#pragma once 

#include "Shader.h"

// std 
#include <glad/glad.h>
#include <string>

// Declare Container with these parmeters  
class Triangle {

public:

	Triangle();
	~Triangle();


	void Draw() const;



private:
	// variables 
	unsigned int VAO, VBO, EBO, textureID;

	Shader* myShader;

};