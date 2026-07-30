#pragma once 

// std 
#include <glad/glad.h>
#include <string>

// Declare Container with these parmeters  
class Triangle {
private:
	// variables 
	unsigned int VAO; // variable to cpu ID
	unsigned int VBO;  // variable to cpu ID 
	unsigned int EBO;
	unsigned int shaderProgram;

	// Helper function 
	std::string readShaderFile(const char* filepath);
	void compileShaders();

public:
	// Constructor complier knows that the moment someone types triangle my triangle , 
	// initialize routine must be kicked off 

	Triangle(); // Constructor Generates the buffers and sends vertex data to the GPU


	// Destructor complier knows to cleanup ensuring no leaked memory left in the gpu 
	~Triangle(); // Destructor Cleans up the GPU memory when the object is destroyed


	// when main loop hits this command trigger rendering 
	void Draw() const; // Binds the VAO and executes the draw call 
};