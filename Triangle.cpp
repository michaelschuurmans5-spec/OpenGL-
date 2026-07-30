#include "Triangle.h"

// std 
#include <fstream>
#include <sstream>
#include <iostream>
#include <glad/glad.h> 


Triangle::Triangle() {
	// 1. LOAD AND COMPILE SHADERS 
	std::string vertexCode;
	std::string fragmentCode; 

	std::ifstream vShaderFile("Assets/Shaders/Shapes/Triangle.vert");
	std::ifstream fShaderFile("Assets/Shaders/Shapes/Triangle.frag"); // Open the fragment shader file

	std::stringstream vShaderStream, fShaderStream;

	// Read Vertex File
	if (vShaderFile.is_open()) {
		vShaderStream << vShaderFile.rdbuf();
		vShaderFile.close();
		vertexCode = vShaderStream.str();
	}
	else {
		std::cerr << "ERROR: Vertex Shader file not found!" << std::endl;
	}

	// Read Fragment File
	if (fShaderFile.is_open()) {
		fShaderStream << fShaderFile.rdbuf();
		fShaderFile.close();
		fragmentCode = fShaderStream.str();
	}
	else {
		std::cerr << "ERROR: Fragment Shader file not found!" << std::endl;
	}

	const char* vShaderSource = vertexCode.c_str();
	const char* fShaderSource = fragmentCode.c_str();

	int success;
	char infoLog[512];

	// Compile Vertex Shader
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vShaderSource, NULL);
	glCompileShader(vertexShader);

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cerr << "ERROR: Vertex Shader Compilation Failed\n" << infoLog << std::endl;
	}

	// Compile Fragment Shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fShaderSource, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cerr << "ERROR: Fragment Shader Compilation Failed\n" << infoLog << std::endl;
	}

	// Create and Link Program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader); // Attach BOTH shaders together!
	glLinkProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cerr << "ERROR: Shader Program Linking Failed\n" << infoLog << std::endl;
	}

	// Clean up shader objects now that they are linked
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader); // Delete fragment object reference

	// 2. SET UP VAO, VBO, AND EBO DATA
	float vertices[] = {
		 0.5f,  0.5f, 0.0f,  // Top Right (Index 0)
		 0.5f, -0.5f, 0.0f,  // Bottom Right (Index 1)
		-0.5f, -0.5f, 0.0f,  // Bottom Left (Index 2)
		-0.5f,  0.5f, 0.0f   // Top Left (Index 3)
	};

	unsigned int indices[] = {
		0, 1, 3,  // First Triangle
		1, 2, 3   // Second Triangle
	};

	// allocate Containers inside vram
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Bind Containers VAO + VBO
	glBindVertexArray(VAO);
	// take id VBO and plug it into array container slot 
	glBindBuffer(GL_ARRAY_BUFFER, VBO);  

	// sends vertices array to the GPU so it can match correct size 
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// take buffer ID stored in EBO variable and plug it into GL_ELEMENT_ARRAY slot 
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	// sends indices to the GPU so it can match correct size 
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// VAO manger how to interpet and read those active slots 
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (GLsizei)(3 * sizeof(float)), (void*)0);
	glEnableVertexAttribArray(0);

	// Unplug VBO from the GL_ARRAY_BUFFER slot 
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// Unplug VAO manager 
	glBindVertexArray(0);
}

// 3. CLEAN UP ALL MEMORY BUFFERS
Triangle::~Triangle() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);           //  Cleaned up here
	glDeleteProgram(shaderProgram);     //  Cleaned up here
}

// 4. DRAW USING ELEMENTS
void Triangle::Draw() const {
	glUseProgram(shaderProgram);

	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}
