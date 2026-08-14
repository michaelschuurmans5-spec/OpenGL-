#pragma once 


#include "Shader.h"
#include <glm/glm/glm.hpp>

class Sky {
public:
	Sky();
	~Sky();


	void Draw(
		const glm::mat4& view,
		const glm::mat4& projection,
		const glm::vec3& sunDirection
	)const;


	// Sky Settings

	struct settings {
		float skyBrightness = 1.0f;
		float horizonHaze = 0.65f;

		// Cloud settings will be added in step 4
		float cloudCoverage = 0.55f;
		float cloudDensity = 0.70f;
		float cloudSpeed = 0.025f;
		float cloudHeight = 0.60f;

		bool cloudsEnabled = true;

	};

	settings settings;

private:

	Shader* skyShader = nullptr;

	unsigned int VAO = 0;
	unsigned int VBO = 0;
};