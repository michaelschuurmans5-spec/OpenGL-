#include "Triangle.h"
#include "stb_image.h"

// std 
#include <fstream>
#include <sstream>
#include <iostream>
#include <glad/glad.h> 


Triangle::Triangle() {

	// Shader loading and compiling 
	myShader = new Shader("Assets/Shaders/Shapes/Triangle.vert", "Assets/Shaders/Shapes/Triangle.frag");
	

	// 2. SET UP VAO, VBO, AND EBO DATA
	float vertices[] = {
		// Positions          // Colors           // Texture Coords (UV)
	  0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // Top Right (Index 0)
	  0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // Bottom Right (Index 1)
	 -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // Bottom Left (Index 2)
	 -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // Top Left (Index 3)
	};

	unsigned int indices[] = {
		0, 1, 3, // First Triangle
	    1, 2, 3  // Second Triangle 
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

	// Stride is 8 floats total (3 position + 3 color + 2 texture)
	GLsizei stride = 8 * sizeof(float);

	// Positions attribute Location 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);
	// color attribute Location 1 
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Attribute Location 2: Texture Coords (UV)
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// Generate and config the GL Texture
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Set texture wrapping to repeat
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set texture filtering configuration rules
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Load The Image Data
	int width, height, nrChannels;
	// Tell stb_image to flip the image vertically (OpenGL uses 0,0 at bottom-left)
	stbi_set_flip_vertically_on_load(true);

	// Texture File Path
	unsigned char* data = stbi_load("Assets/Textures/Shapes/ShapeTxt/container.jpg", &width, &height, &nrChannels, 0);

	if (data) {
		// Use GL_RGB for .jpg files. Change to GL_RGBA if you use a transparent .png file!
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cerr << "ERROR: FAILED TO LOAD TEXTURE FROM DISK!" << std::endl;
	}

	// Always clear RAM buffer once image is securely up in GPU memory buffer space
	stbi_image_free(data);

	// Unplug VBO from the GL_ARRAY_BUFFER slot 
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// Unplug VAO manager 
	glBindVertexArray(0);
}

// 3. CLEAN UP ALL MEMORY BUFFERS
Triangle::~Triangle() {
	delete myShader;
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO); 
	glDeleteTextures(1, &textureID);
	
}

// 4. DRAW USING ELEMENTS
void Triangle::Draw() const {
	myShader->use();

	// Bind the texture to the active unit array right before rendering
	glBindTexture(GL_TEXTURE_2D, textureID);

	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}