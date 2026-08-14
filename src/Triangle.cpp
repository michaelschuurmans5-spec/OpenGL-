#include "Triangle.h"
#include "stb_image.h"
#include "Sky.h"

// Standard C++ 
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <cmath>

// Third party
#include <GLFW/glfw3.h>
#include <glad/glad.h> 
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp> // Critical for glm::translate / glm::scale
#include <glm/glm/gtc/type_ptr.hpp>

namespace {
	// All generated meshes use the same 8-float stride as the cube
	// (position3, normal3, texcoord2) so they work with the existing shader.
	constexpr GLsizei kMeshStride = 8 * sizeof(float);

	void SetupMeshAttribs() {
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kMeshStride, (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, kMeshStride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, kMeshStride, (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);
	}

	void UploadMesh(unsigned int& outVAO, unsigned int& outVBO, unsigned int& outEBO,
		const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {

		glGenVertexArrays(1, &outVAO);
		glGenBuffers(1, &outVBO);
		glGenBuffers(1, &outEBO);

		glBindVertexArray(outVAO);
		glBindBuffer(GL_ARRAY_BUFFER, outVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

		SetupMeshAttribs();
		glBindVertexArray(0);
	}

	// Unit-radius-0.5 UV sphere, Y-up.
	void CreateSphereMesh(unsigned int& outVAO, unsigned int& outVBO, unsigned int& outEBO, unsigned int& outIndexCount) {
		const unsigned int sectorCount = 24;
		const unsigned int stackCount = 16;
		const float radius = 0.5f;
		const float PI = 3.14159265359f;

		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		for (unsigned int i = 0; i <= stackCount; ++i) {
			float phi = PI / 2.0f - i * (PI / stackCount); // +90 down to -90
			float y = radius * sinf(phi);
			float r = radius * cosf(phi);

			for (unsigned int j = 0; j <= sectorCount; ++j) {
				float theta = j * (2.0f * PI / sectorCount);
				float x = r * cosf(theta);
				float z = r * sinf(theta);

				vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
				glm::vec3 n = glm::normalize(glm::vec3(x, y, z));
				vertices.push_back(n.x); vertices.push_back(n.y); vertices.push_back(n.z);
				vertices.push_back((float)j / sectorCount); vertices.push_back((float)i / stackCount);
			}
		}

		for (unsigned int i = 0; i < stackCount; ++i) {
			unsigned int k1 = i * (sectorCount + 1);
			unsigned int k2 = k1 + sectorCount + 1;

			for (unsigned int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
				if (i != 0) {
					indices.push_back(k1);
					indices.push_back(k2);
					indices.push_back(k1 + 1);
				}
				if (i != stackCount - 1) {
					indices.push_back(k1 + 1);
					indices.push_back(k2);
					indices.push_back(k2 + 1);
				}
			}
		}

		outIndexCount = (unsigned int)indices.size();
		UploadMesh(outVAO, outVBO, outEBO, vertices, indices);
	}

	// Radius-0.5, height-1 cylinder, Y-up, with smooth side normals and flat cap normals.
	void CreateCylinderMesh(unsigned int& outVAO, unsigned int& outVBO, unsigned int& outEBO, unsigned int& outIndexCount) {
		const unsigned int sectorCount = 24;
		const float radius = 0.5f;
		const float halfHeight = 0.5f;
		const float PI = 3.14159265359f;

		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		// Side wall (two rings, radial normals)
		for (unsigned int ring = 0; ring < 2; ++ring) {
			float y = (ring == 0) ? -halfHeight : halfHeight;
			for (unsigned int j = 0; j <= sectorCount; ++j) {
				float theta = j * (2.0f * PI / sectorCount);
				float x = radius * cosf(theta);
				float z = radius * sinf(theta);

				vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
				glm::vec3 n = glm::normalize(glm::vec3(x, 0.0f, z));
				vertices.push_back(n.x); vertices.push_back(n.y); vertices.push_back(n.z);
				vertices.push_back((float)j / sectorCount); vertices.push_back((float)ring);
			}
		}
		unsigned int ringVerts = sectorCount + 1;
		for (unsigned int j = 0; j < sectorCount; ++j) {
			unsigned int b0 = j, b1 = j + 1;
			unsigned int t0 = ringVerts + j, t1 = ringVerts + j + 1;
			indices.push_back(b0); indices.push_back(t0); indices.push_back(b1);
			indices.push_back(b1); indices.push_back(t0); indices.push_back(t1);
		}

		// Bottom cap
		unsigned int bottomCenter = (unsigned int)(vertices.size() / 8);
		vertices.push_back(0.0f); vertices.push_back(-halfHeight); vertices.push_back(0.0f);
		vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
		vertices.push_back(0.5f); vertices.push_back(0.5f);

		unsigned int bottomRim = (unsigned int)(vertices.size() / 8);
		for (unsigned int j = 0; j <= sectorCount; ++j) {
			float theta = j * (2.0f * PI / sectorCount);
			float x = radius * cosf(theta);
			float z = radius * sinf(theta);
			vertices.push_back(x); vertices.push_back(-halfHeight); vertices.push_back(z);
			vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
			vertices.push_back(0.5f + 0.5f * cosf(theta)); vertices.push_back(0.5f + 0.5f * sinf(theta));
		}
		for (unsigned int j = 0; j < sectorCount; ++j) {
			indices.push_back(bottomCenter);
			indices.push_back(bottomRim + j + 1);
			indices.push_back(bottomRim + j);
		}

		// Top cap
		unsigned int topCenter = (unsigned int)(vertices.size() / 8);
		vertices.push_back(0.0f); vertices.push_back(halfHeight); vertices.push_back(0.0f);
		vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
		vertices.push_back(0.5f); vertices.push_back(0.5f);

		unsigned int topRim = (unsigned int)(vertices.size() / 8);
		for (unsigned int j = 0; j <= sectorCount; ++j) {
			float theta = j * (2.0f * PI / sectorCount);
			float x = radius * cosf(theta);
			float z = radius * sinf(theta);
			vertices.push_back(x); vertices.push_back(halfHeight); vertices.push_back(z);
			vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
			vertices.push_back(0.5f + 0.5f * cosf(theta)); vertices.push_back(0.5f + 0.5f * sinf(theta));
		}
		for (unsigned int j = 0; j < sectorCount; ++j) {
			indices.push_back(topCenter);
			indices.push_back(topRim + j);
			indices.push_back(topRim + j + 1);
		}

		outIndexCount = (unsigned int)indices.size();
		UploadMesh(outVAO, outVBO, outEBO, vertices, indices);
	}

	// Unit quad on the XZ plane, facing +Y - a placeable "Plane" primitive,
	// distinct from the large static Ground mesh already in the scene.
	void CreatePlaneMesh(unsigned int& outVAO, unsigned int& outVBO, unsigned int& outEBO) {
		std::vector<float> vertices = {
			-0.5f, 0.0f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
			 0.5f, 0.0f, -0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
			 0.5f, 0.0f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
			-0.5f, 0.0f,  0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f
		};
		std::vector<unsigned int> indices = { 0, 1, 2, 0, 2, 3 };

		UploadMesh(outVAO, outVBO, outEBO, vertices, indices);
	}

	// Triangular prism (equilateral cross-section), Y-up, flat-shaded faces.
	void CreatePrismMesh(unsigned int& outVAO, unsigned int& outVBO, unsigned int& outEBO, unsigned int& outIndexCount) {
		const float halfHeight = 0.5f;
		glm::vec2 p0(0.0f, 0.5f);
		glm::vec2 p1(-0.433f, -0.25f);
		glm::vec2 p2(0.433f, -0.25f);

		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		auto pushVert = [&](const glm::vec3& pos, const glm::vec3& normal, const glm::vec2& uv) {
			vertices.push_back(pos.x); vertices.push_back(pos.y); vertices.push_back(pos.z);
			vertices.push_back(normal.x); vertices.push_back(normal.y); vertices.push_back(normal.z);
			vertices.push_back(uv.x); vertices.push_back(uv.y);
			};

		auto addTri = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
			glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
			unsigned int start = (unsigned int)(vertices.size() / 8);
			pushVert(a, n, glm::vec2(0.0f, 0.0f));
			pushVert(b, n, glm::vec2(1.0f, 0.0f));
			pushVert(c, n, glm::vec2(0.5f, 1.0f));
			indices.push_back(start); indices.push_back(start + 1); indices.push_back(start + 2);
			};

		auto addQuad = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d) {
			glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
			unsigned int start = (unsigned int)(vertices.size() / 8);
			pushVert(a, n, glm::vec2(0.0f, 0.0f));
			pushVert(b, n, glm::vec2(1.0f, 0.0f));
			pushVert(c, n, glm::vec2(1.0f, 1.0f));
			pushVert(d, n, glm::vec2(0.0f, 1.0f));
			indices.push_back(start); indices.push_back(start + 1); indices.push_back(start + 2);
			indices.push_back(start); indices.push_back(start + 2); indices.push_back(start + 3);
			};

		glm::vec3 b0(p0.x, -halfHeight, p0.y), b1(p1.x, -halfHeight, p1.y), b2(p2.x, -halfHeight, p2.y);
		glm::vec3 t0(p0.x, halfHeight, p0.y), t1(p1.x, halfHeight, p1.y), t2(p2.x, halfHeight, p2.y);

		addTri(b0, b2, b1);   // bottom cap
		addTri(t0, t1, t2);   // top cap
		addQuad(b0, b1, t1, t0);
		addQuad(b1, b2, t2, t1);
		addQuad(b2, b0, t0, t2);

		outIndexCount = (unsigned int)indices.size();
		UploadMesh(outVAO, outVBO, outEBO, vertices, indices);
	}

	// -----------------------------------------------------------------
	// Level Designer > Terrain Generator
	// -----------------------------------------------------------------

	// Deterministic 2D hash so the same seed always produces the same
	// terrain layout, and different seeds produce different layouts.
	float Hash2D(int x, int y, int seed) {
		unsigned int h = (unsigned int)(x * 374761393 + y * 668265263 + seed * 2147483647u);
		h = (h ^ (h >> 13)) * 1274126177u;
		h = h ^ (h >> 16);
		return (float)(h & 0xFFFFFF) / (float)0xFFFFFF; // [0,1)
	}

	// Smoothly-interpolated value noise built on top of Hash2D.
	float SmoothNoise2D(float x, float y, int seed) {
		int x0 = (int)floorf(x), y0 = (int)floorf(y);
		int x1 = x0 + 1, y1 = y0 + 1;
		float tx = x - x0, ty = y - y0;

		float h00 = Hash2D(x0, y0, seed);
		float h10 = Hash2D(x1, y0, seed);
		float h01 = Hash2D(x0, y1, seed);
		float h11 = Hash2D(x1, y1, seed);

		float sx = tx * tx * (3.0f - 2.0f * tx); // smoothstep
		float sy = ty * ty * (3.0f - 2.0f * ty);

		float top = h00 + (h10 - h00) * sx;
		float bottom = h01 + (h11 - h01) * sx;
		return (top + (bottom - top) * sy) * 2.0f - 1.0f; // [-1,1]
	}

	// Fractal Brownian Motion: several octaves of SmoothNoise2D layered
	// together for natural-looking rolling terrain (drives "Hills").
	float FBM(float x, float y, int seed, int octaves) {
		float total = 0.0f, amplitude = 1.0f, frequency = 1.0f, maxAmp = 0.0f;
		for (int i = 0; i < octaves; i++) {
			total += SmoothNoise2D(x * frequency, y * frequency, seed + i * 101) * amplitude;
			maxAmp += amplitude;
			amplitude *= 0.5f;
			frequency *= 2.0f;
		}
		return maxAmp > 0.0f ? total / maxAmp : 0.0f; // [-1,1]
	}

	// Ridged noise: folds the noise so it always peaks upward, giving the
	// jagged look used by "Mountains".
	float RidgedNoise(float x, float y, int seed, int octaves) {
		float total = 0.0f, amplitude = 1.0f, frequency = 1.0f, maxAmp = 0.0f;
		for (int i = 0; i < octaves; i++) {
			float n = 1.0f - fabsf(SmoothNoise2D(x * frequency, y * frequency, seed + i * 53));
			total += n * n * amplitude;
			maxAmp += amplitude;
			amplitude *= 0.5f;
			frequency *= 2.0f;
		}
		return maxAmp > 0.0f ? total / maxAmp : 0.0f; // [0,1]
	}

	// Computes terrain height at a normalized grid position (u,v in [0,1])
	// from the Level Designer sliders. Every feature is additive or
	// subtractive so dialing its slider to 0 removes it entirely.
	float ComputeTerrainHeight(float u, float v, const TerrainParams& params) {
		float height = 0.0f;

		// Hills: broad, low-frequency rolling elevation
		height += FBM(u * 3.0f, v * 3.0f, params.seed, 4) * params.hillScale;

		// Mountains: ridged, higher-frequency sharp peaks
		if (params.mountainScale > 0.0f) {
			float ridge = RidgedNoise(u * 2.0f, v * 2.0f, params.seed + 500, 4);
			height += ridge * ridge * params.mountainScale * 3.0f;
		}

		// Valleys: broad, low-frequency depressions cut into the base
		if (params.valleyScale > 0.0f) {
			float dip = FBM(u * 1.5f, v * 1.5f, params.seed + 900, 3);
			height -= fabsf(dip) * params.valleyScale * 2.0f;
		}

		// Holes: sparse, localized circular pits scattered across the grid
		if (params.holeScale > 0.0f) {
			const int holeCount = 10;
			for (int i = 0; i < holeCount; i++) {
				float hx = Hash2D(i, 17, params.seed + 1300);
				float hy = Hash2D(i, 91, params.seed + 1300);
				float dx = u - hx, dy = v - hy;
				float dist = sqrtf(dx * dx + dy * dy);
				const float radius = 0.06f;
				if (dist < radius) {
					float falloff = 1.0f - (dist / radius);
					height -= falloff * falloff * params.holeScale * 2.0f;
				}
			}
		}

		// Rocks: high-frequency surface roughness on top of everything else
		if (params.rockScale > 0.0f) {
			height += FBM(u * 20.0f, v * 20.0f, params.seed + 2200, 2) * params.rockScale * 0.3f;
		}

		return height;
	}

	// Builds/rebuilds a resolution x resolution grid mesh on the XZ plane,
	// sized size x size and centered at the origin, with heights driven by
	// ComputeTerrainHeight() and normals from central-difference gradients.
	// If outVAO already exists, its buffers are re-uploaded in place
	// (cheap - no new GL objects) so live slider dragging doesn't leak VAOs.
	void CreateTerrainMesh(unsigned int& outVAO, unsigned int& outVBO, unsigned int& outEBO,
		unsigned int& outIndexCount, const TerrainParams& params) {

		int res = params.resolution < 2 ? 2 : params.resolution;
		float size = params.size;
		float half = size * 0.5f;
		float cellSize = size / res;

		std::vector<float> heights((size_t)(res + 1) * (res + 1));
		for (int iz = 0; iz <= res; iz++) {
			for (int ix = 0; ix <= res; ix++) {
				float u = (float)ix / res;
				float v = (float)iz / res;
				heights[iz * (res + 1) + ix] = ComputeTerrainHeight(u, v, params);
			}
		}

		std::vector<float> vertices;
		vertices.reserve((size_t)(res + 1) * (res + 1) * 8);

		for (int iz = 0; iz <= res; iz++) {
			for (int ix = 0; ix <= res; ix++) {
				float x = -half + ix * cellSize;
				float z = -half + iz * cellSize;
				float y = heights[iz * (res + 1) + ix];

				float hL = heights[iz * (res + 1) + (ix > 0 ? ix - 1 : ix)];
				float hR = heights[iz * (res + 1) + (ix < res ? ix + 1 : ix)];
				float hD = heights[(iz > 0 ? iz - 1 : iz) * (res + 1) + ix];
				float hU = heights[(iz < res ? iz + 1 : iz) * (res + 1) + ix];

				glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f * cellSize, hD - hU));

				vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
				vertices.push_back(normal.x); vertices.push_back(normal.y); vertices.push_back(normal.z);
				vertices.push_back((float)ix / res * 8.0f); vertices.push_back((float)iz / res * 8.0f);
			}
		}

		std::vector<unsigned int> indices;
		indices.reserve((size_t)res * res * 6);
		for (int iz = 0; iz < res; iz++) {
			for (int ix = 0; ix < res; ix++) {
				unsigned int i0 = iz * (res + 1) + ix;
				unsigned int i1 = i0 + 1;
				unsigned int i2 = i0 + (res + 1);
				unsigned int i3 = i2 + 1;

				indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
				indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
			}
		}

		outIndexCount = (unsigned int)indices.size();

		if (outVAO == 0) {
			glGenVertexArrays(1, &outVAO);
			glGenBuffers(1, &outVBO);
			glGenBuffers(1, &outEBO);
		}

		glBindVertexArray(outVAO);
		glBindBuffer(GL_ARRAY_BUFFER, outVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);
		SetupMeshAttribs();
		glBindVertexArray(0);
	}
}

Triangle::Triangle() {

	// Shader loading and compiling 
	myShader = new Shader("Assets/Shaders/Shapes/Triangle.vert", "Assets/Shaders/Shapes/Triangle.frag");
	lightCubeShader = new Shader("Assets/Shaders/Shapes/LightCube.vert", "Assets/Shaders/Shapes/LightCube.frag");

	csmShader = new Shader("Assets/Shaders/Lighting/Shadows/csm.vert",
		"Assets/Shaders/Lighting/Shadows/csm.frag",
		"Assets/Shaders/Lighting/Shadows/csm.geom"); // Note: Pass geometry shader path if Shader class supports 3 paths

	myShader = new Shader(
		"Assets/Shaders/Shapes/Triangle.vert",
		"Assets/Shaders/Shapes/Triangle.frag"
	);

	lightCubeShader = new Shader(
		"Assets/Shaders/Shapes/LightCube.vert",
		"Assets/Shaders/Shapes/LightCube.frag"
	);

	csmShader = new Shader(
		"Assets/Shaders/Lighting/Shadows/csm.vert",
		"Assets/Shaders/Lighting/Shadows/csm.frag",
		"Assets/Shaders/Lighting/Shadows/csm.geom"
	);

	sky = new Sky();


	// SET UP DATA (Positions, Normals, TexCoords)
	float vertices[] = {
		// Positions          // Normals           // TexCoords
		// Back Face
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,

		// Front Face
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,

		// Left Face
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		// Right Face
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		 // Bottom Face
		 -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
		  0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
		 -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

		 // Top Face
		 -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
		  0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
		 -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
	};

	unsigned int indices[] = {
		0,  2,  1,  0,  3,  2,  // Back
		4,  5,  6,  4,  6,  7,  // Front
		8,  9,  10, 8,  10, 11, // Left
		12, 14, 13, 12, 15, 14, // Right
		16, 17, 18, 16, 18, 19, // Bottom
		20, 22, 21, 20, 23, 22  // Top
	};

	// Allocate containers inside VRAM
	glGenVertexArrays(1, &VAO);
	glGenVertexArrays(1, &lightVAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Stride for the cube: 8 floats
	GLsizei stride = 8 * sizeof(float);

	// A. CONFIGURE MAIN OBJECT VAO
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Positions attribute Location 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);
	// Normal attribute Location 1 
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// TexCoords attribute location 
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

	// B. CONFIGURE LIGHT SOURCE VAO (Using same VBO/EBO)
	glBindVertexArray(lightVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	// The light source cube only needs the positions vector attribute layout (Location 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	// C. CONFIGURE GROUND PLANE VAO
	float groundVertices[] = {
		// positions            // normals        // TexCoords
		-5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
		 5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,   5.0f, 0.0f,
		 5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,   5.0f, 5.0f,
		-5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 5.0f
	};
	unsigned int groundIndices[] = { 0, 1, 2, 0, 2, 3 };

	GLsizei groundStride = 8 * sizeof(float);

	glGenVertexArrays(1, &groundVAO);
	glGenBuffers(1, &groundVBO);
	glGenBuffers(1, &groundEBO);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);

	// GROUND TEXTURE - Grass
	glGenTextures(1, &groundTexture);
	glBindTexture(GL_TEXTURE_2D, groundTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	unsigned char* data = stbi_load("Assets/Textures/Shapes/ShapeTxt/BasicGrass.jpg", &width, &height, &nrChannels, 0);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cerr << "ERROR: FAILED TO LOAD GRASS TEXTURE FROM DISK!" << std::endl;
	}
	stbi_image_free(data);

	// CUBE TEXTURE - Container
	glGenTextures(1, &cubeTexture);
	glBindTexture(GL_TEXTURE_2D, cubeTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	data = stbi_load("Assets/Textures/Shapes/ShapeTxt/container.jpg", &width, &height, &nrChannels, 0);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cerr << "ERROR: FAILED TO LOAD CUBE CONTAINER TEXTURE FROM DISK!" << std::endl;
	}
	stbi_image_free(data);

	
	

	// Unbind everything safely
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// D. GENERATE BASIC-SHAPE MESHES (Sphere, Cylinder, Plane, Prism)
	// used by the Objects > Shapes drag-and-drop panel.
	CreateSphereMesh(sphereVAO, sphereVBO, sphereEBO, sphereIndexCount);
	CreateCylinderMesh(cylinderVAO, cylinderVBO, cylinderEBO, cylinderIndexCount);
	CreatePlaneMesh(planeVAO, planeVBO, planeEBO);
	CreatePrismMesh(prismVAO, prismVBO, prismEBO, prismIndexCount);

	glBindVertexArray(0);
}

// Parameter naming fixed to match standard spelling rules + added matrix baking
void Triangle::SpawnCube(const glm::vec3& spawnPosition) {
	std::string name = "Cube_" + std::to_string(nextCubeID);
	nextCubeID++;

	GameObject cube{};
	cube.type = ObjectType::Cube;
	cube.position = spawnPosition;
	cube.scale = glm::vec3(1.0f);
	cube.name = name;
	cube.rotates = false;
	cube.rotation = glm::vec3(0.0f);
	cube.transformMatrix = glm::translate(glm::mat4(1.0f), spawnPosition); // FIXED: Passing 2 parameters explicitly
	cube.textureSlot = TextureSlot::Container;
	sceneObjects.push_back(cube);
}

// Generic spawn used by the Objects > Shapes properties panel's "Create"
// button and by dropping a shape from that menu into the viewport.
// Each ObjectType is drawn with its own dedicated mesh (see Draw()).
void Triangle::SpawnShape(ObjectType type, const glm::vec3& spawnPosition, const std::string& customName,
	const glm::vec3& baseColor, TextureSlot textureSlot) {

	std::string name = customName.empty()
		? (ToString(type) + "_" + std::to_string(nextCubeID))
		: customName;
	nextCubeID++;

	GameObject obj{};
	obj.type = type;
	obj.position = spawnPosition;
	obj.scale = glm::vec3(1.0f);
	obj.name = name;
	obj.rotates = false;
	obj.rotation = glm::vec3(0.0f);
	obj.transformMatrix = glm::translate(glm::mat4(1.0f), spawnPosition);
	obj.baseColor = baseColor;
	obj.textureSlot = textureSlot;
	obj.isBasicShape = true;

	sceneObjects.push_back(obj);
}

// ---------------------------------------------------------------------
// Level Designer > Terrain Generator
// ---------------------------------------------------------------------

void Triangle::PreviewTerrain(const TerrainParams& params) {
	terrainParams = params;
	CreateTerrainMesh(terrainVAO, terrainVBO, terrainEBO, terrainIndexCount, terrainParams);
	// If a terrain is already committed, it renders through the sceneObjects
	// loop below using this same VAO, so the change is visible immediately -
	// that's the "see it happen in the viewport" live preview.
}

void Triangle::CommitTerrain() {
	if (terrainIndexCount == 0) return; // nothing generated yet - Generate has no effect
	if (terrainObjectId != -1) return;  // already committed, nothing further to add

	GameObject obj{};
	obj.type = ObjectType::Terrain;
	obj.position = glm::vec3(0.0f);
	obj.scale = glm::vec3(1.0f);
	obj.name = "Terrain";
	obj.rotates = false;
	obj.rotation = glm::vec3(0.0f);
	obj.transformMatrix = glm::mat4(1.0f);
	obj.baseColor = glm::vec3(0.35f, 0.55f, 0.25f); // grassy green default
	obj.textureSlot = TextureSlot::None;
	obj.isBasicShape = false;

	sceneObjects.push_back(obj);
	terrainObjectId = obj.id;
}

void Triangle::DeleteTerrain() {
	if (terrainObjectId != -1) {
		for (size_t i = 0; i < sceneObjects.size(); i++) {
			if (sceneObjects[i].id == terrainObjectId) {
				sceneObjects.erase(sceneObjects.begin() + i);
				break;
			}
		}
		terrainObjectId = -1;
	}

	if (terrainVAO != 0) {
		glDeleteVertexArrays(1, &terrainVAO);
		glDeleteBuffers(1, &terrainVBO);
		glDeleteBuffers(1, &terrainEBO);
		terrainVAO = terrainVBO = terrainEBO = 0;
	}
	terrainIndexCount = 0;
}

void Triangle::OnObjectDeleted(int objectId) {
	if (objectId == terrainObjectId) {
		// The GameObject was removed by the generic Viewport Manager delete
		// flow rather than the panel's own "Delete Terrain" button. Drop the
		// commit tracking but keep the mesh around as an uncommitted preview
		// (Draw() will keep showing it until Delete Terrain is pressed, or
		// it gets committed again).
		terrainObjectId = -1;
	}
}

// Added matrix baking configuration to light entities
void Triangle::SpawnLight(const glm::vec3& spawnPosition) {
	std::string name = "Light_" + std::to_string(nextLightID);
	nextLightID++;

	GameObject light{};
	light.type = ObjectType::Light;
	light.position = spawnPosition;
	light.scale = glm::vec3(1.0f);
	light.name = name;
	light.rotates = false;
	light.rotation = glm::vec3(0.0f);
	light.transformMatrix = glm::translate(glm::mat4(1.0f), spawnPosition); // FIXED: Passing 2 parameters explicitly
	sceneObjects.push_back(light);
}

Triangle::~Triangle() {
	delete myShader;
	delete lightCubeShader;

	delete myShader;
	delete lightCubeShader;
	delete sky;

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	glDeleteTextures(1, &groundTexture);
	glDeleteTextures(1, &cubeTexture);

	glDeleteVertexArrays(1, &sphereVAO);
	glDeleteBuffers(1, &sphereVBO);
	glDeleteBuffers(1, &sphereEBO);

	glDeleteVertexArrays(1, &cylinderVAO);
	glDeleteBuffers(1, &cylinderVBO);
	glDeleteBuffers(1, &cylinderEBO);

	glDeleteVertexArrays(1, &planeVAO);
	glDeleteBuffers(1, &planeVBO);
	glDeleteBuffers(1, &planeEBO);

	glDeleteVertexArrays(1, &prismVAO);
	glDeleteBuffers(1, &prismVBO);
	glDeleteBuffers(1, &prismEBO);

	if (terrainVAO != 0) {
		glDeleteVertexArrays(1, &terrainVAO);
		glDeleteBuffers(1, &terrainVBO);
		glDeleteBuffers(1, &terrainEBO);
	}
}

void Triangle::Draw(const glm::mat4& viewMatrix,
	const glm::mat4& projectionMatrix,
	const glm::vec3& lightPos,
	const glm::vec3& cameraPos) const
{
	// 1. Get main viewport dimensions
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	int currentWidth = viewport[2];
	int currentHeight = viewport[3];

	const int MAX_LIGHTS = 8;
	std::vector<glm::vec3> activeLightPositions;

	for (const auto& obj : sceneObjects) {
		if (!obj.visible) continue;
		if (obj.type == ObjectType::Light) {
			activeLightPositions.push_back(obj.position);
			if (activeLightPositions.size() >= MAX_LIGHTS) break;
		}
	}

	bool hasLightEntity = !activeLightPositions.empty();
	// Use ambient directional light settings if no lights are spawned in the scene
	glm::vec3 mainLightDir = GetSunDirection();

	if (!activeLightPositions.empty()) {
		glm::vec3 lightPos = activeLightPositions[0];
		// Ensure position isn't zero vector before normalizing
		if (glm::length(lightPos) > 0.0001f) {
			mainLightDir = glm::normalize(lightPos);
		}
	}

	// Viewport & Frustum parameters
	float nearPlane = 0.1f;
	float farPlane = 500.0f;
	float fovDeg = 45.0f;
	float aspect = (currentHeight > 0) ? static_cast<float>(currentWidth) / static_cast<float>(currentHeight) : 1.0f;

	// ====================================================
	// PASS 0: CSM DEPTH PASS
	// ====================================================
	if (shadowMap && csmShader) {
		shadowMap->UpdateCascades(
			nearPlane, farPlane, fovDeg, aspect, mainLightDir, viewMatrix
		);

		// Guard against empty cascade matrices
		if (!shadowMap->shadowMatrices.empty()) {
			// Bind Shadow Framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, shadowMap->fbo);
			glViewport(0, 0, shadowMap->shadowResolution, shadowMap->shadowResolution);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			glClear(GL_DEPTH_BUFFER_BIT);

			// DISABLE CULLING FOR DEPTH PASS TO PREVENT MISSING SHADOWS
			glDisable(GL_CULL_FACE);

			csmShader->use();

			for (size_t i = 0; i < shadowMap->shadowMatrices.size(); ++i) {
				std::string uniformName = "shadowMatrices[" + std::to_string(i) + "]";
				csmShader->setMat4(uniformName, shadowMap->shadowMatrices[i]);
			}

			// Restore default culling if you use it in main pass
			glEnable(GL_CULL_FACE);

			int csmModelLoc = glGetUniformLocation(csmShader->ID, "model");

			// Draw Depth Geometry
			for (const auto& obj : sceneObjects) {
				if (!obj.visible || obj.type == ObjectType::Light) continue;

				glUniformMatrix4fv(csmModelLoc, 1, GL_FALSE, glm::value_ptr(obj.transformMatrix));

				if (obj.type == ObjectType::Ground) {
					glBindVertexArray(groundVAO);
					glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
				}
				else {
					switch (obj.type) {
					case ObjectType::Sphere:
						glBindVertexArray(sphereVAO);
						glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
						break;
					case ObjectType::Cylinder:
						glBindVertexArray(cylinderVAO);
						glDrawElements(GL_TRIANGLES, cylinderIndexCount, GL_UNSIGNED_INT, 0);
						break;
					case ObjectType::Plane:
						glBindVertexArray(planeVAO);
						glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
						break;
					case ObjectType::Prism:
						glBindVertexArray(prismVAO);
						glDrawElements(GL_TRIANGLES, prismIndexCount, GL_UNSIGNED_INT, 0);
						break;
					case ObjectType::Terrain:
						glBindVertexArray(terrainVAO);
						glDrawElements(GL_TRIANGLES, terrainIndexCount, GL_UNSIGNED_INT, 0);
						break;
					case ObjectType::Cube:
					default:
						glBindVertexArray(VAO);
						glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
						break;
					}
				}
			}

			// Draw depth for terrain preview
			if (terrainObjectId == -1 && terrainIndexCount > 0) {
				glUniformMatrix4fv(csmModelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
				glBindVertexArray(terrainVAO);
				glDrawElements(GL_TRIANGLES, terrainIndexCount, GL_UNSIGNED_INT, 0);
			}
		}
	}

	// CRITICAL STATE RESTORATION FOR MAIN RENDER PASS
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK); // Explicitly restore color writing to the default screen buffer!
	glViewport(viewport[0], viewport[1], currentWidth, currentHeight);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);


	// SKY PASS
	if (sky)
	{
		// The sky is infinitely far away.
		// We want it to pass when the depth value is equal
		// to the far/background depth.

		glDepthFunc(GL_LEQUAL);

		// We don't want the sky to write depth.
		glDepthMask(GL_FALSE);

		sky->Draw(
			viewMatrix,
			projectionMatrix,
			GetSunDirection()
		);

		// Restore normal depth behaviour for the scene.
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
	}


	// PASS 1: RENDER MAIN SCENE
	
	// Calculate active light direction from ImGui angles
	glm::vec3 activeSunDir = GetSunDirection();

	// Update positions list (Position 0 represents the primary directional light direction projected into world space)
	if (activeLightPositions.empty()) {
		// If no light objects exist, push the primary sun light position as element 0
		activeLightPositions.push_back(cameraPos + activeSunDir * 100.0f);
	}
	else {
		// Overwrite the first light object's position with the active sun position
		activeLightPositions[0] = cameraPos + activeSunDir * 100.0f;
	}

	myShader->use();

	if (shadowMap) {
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D_ARRAY, shadowMap->depthArrayTexture);
		glUniform1i(glGetUniformLocation(myShader->ID, "shadowMapArray"), 5);

		for (size_t i = 0; i < shadowMap->cascadeSplits.size(); ++i) {
			std::string splitUniform = "cascadeSplits[" + std::to_string(i) + "]";
			std::string matrixUniform = "shadowMatrices[" + std::to_string(i) + "]";

			glUniform1f(glGetUniformLocation(myShader->ID, splitUniform.c_str()), shadowMap->cascadeSplits[i]);
			glUniformMatrix4fv(glGetUniformLocation(myShader->ID, matrixUniform.c_str()), 1, GL_FALSE, glm::value_ptr(shadowMap->shadowMatrices[i]));
		}
	}

	glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	// Pass the raw sun direction directly to the shader
	glUniform3fv(glGetUniformLocation(myShader->ID, "mainSunDir"), 1, glm::value_ptr(activeSunDir));

	glUniform3fv(glGetUniformLocation(myShader->ID, "lightColor"), 1, glm::value_ptr(lightSettings.sunColor* lightSettings.sunIntensity));
	glUniform1f(glGetUniformLocation(myShader->ID, "ambientIntensity"), lightSettings.ambientIntensity);
	glUniform1f(glGetUniformLocation(myShader->ID, "shadowBiasMin"), lightSettings.shadowBiasMin);
	glUniform1f(glGetUniformLocation(myShader->ID, "shadowBiasMax"), lightSettings.shadowBiasMax);
	glUniform1i(glGetUniformLocation(myShader->ID, "debugCascades"), lightSettings.debugCascades ? 1 : 0);

	// Now activeLightPositions is GUARANTEED to have at least 1 element, making this call safe:
	glUniform3fv(
		glGetUniformLocation(myShader->ID, "lightPositions"),
		(GLsizei)activeLightPositions.size(),
		glm::value_ptr(activeLightPositions[0]));

	glUniform1i(
		glGetUniformLocation(myShader->ID, "numLights"),
		(GLsizei)activeLightPositions.size()
	);

	glUniform3fv(glGetUniformLocation(myShader->ID, "viewPos"), 1, glm::value_ptr(cameraPos));

	int modelLoc = glGetUniformLocation(myShader->ID, "model");
	int useTexLoc = glGetUniformLocation(myShader->ID, "useTexture");
	int tex1Loc = glGetUniformLocation(myShader->ID, "texture1");
	int objectColorLoc = glGetUniformLocation(myShader->ID, "objectColor");

	for (const auto& obj : sceneObjects) {
		if (!obj.visible) continue;
		if (obj.type == ObjectType::Light) continue;

		glm::mat4 model = obj.transformMatrix;
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		if (obj.type == ObjectType::Ground) {
			glUniform3f(objectColorLoc, 1.0f, 1.0f, 1.0f);
			glUniform1i(useTexLoc, 1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, groundTexture);
			glUniform1i(tex1Loc, 0);

			glBindVertexArray(groundVAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		else {
			glUniform3f(objectColorLoc, obj.baseColor.r, obj.baseColor.g, obj.baseColor.b);

			GLuint texToBind = 0;
			bool useTex = false;
			if (obj.textureSlot == TextureSlot::Container) { texToBind = cubeTexture; useTex = true; }
			else if (obj.textureSlot == TextureSlot::Grass) { texToBind = groundTexture; useTex = true; }

			glUniform1i(useTexLoc, useTex ? 1 : 0);
			if (useTex) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texToBind);
				glUniform1i(tex1Loc, 0);
			}

			switch (obj.type) {
			case ObjectType::Sphere:
				glBindVertexArray(sphereVAO);
				glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
				break;
			case ObjectType::Cylinder:
				glBindVertexArray(cylinderVAO);
				glDrawElements(GL_TRIANGLES, cylinderIndexCount, GL_UNSIGNED_INT, 0);
				break;
			case ObjectType::Plane:
				glBindVertexArray(planeVAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
				break;
			case ObjectType::Prism:
				glBindVertexArray(prismVAO);
				glDrawElements(GL_TRIANGLES, prismIndexCount, GL_UNSIGNED_INT, 0);
				break;
			case ObjectType::Terrain:
				glBindVertexArray(terrainVAO);
				glDrawElements(GL_TRIANGLES, terrainIndexCount, GL_UNSIGNED_INT, 0);
				break;
			case ObjectType::Cube:
			default:
				glBindVertexArray(VAO);
				glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
				break;
			}
		}
	}

	// Live terrain preview
	if (terrainObjectId == -1 && terrainIndexCount > 0) {
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
		glUniform3f(objectColorLoc, 0.35f, 0.55f, 0.25f);
		glUniform1i(useTexLoc, 0);
		glBindVertexArray(terrainVAO);
		glDrawElements(GL_TRIANGLES, terrainIndexCount, GL_UNSIGNED_INT, 0);
	}

	// ====================================================
	// PASS 2: RENDER PHYSICAL LIGHT CUBES
	// ====================================================
	lightCubeShader->use();

	glUniformMatrix4fv(glGetUniformLocation(lightCubeShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(glGetUniformLocation(lightCubeShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	int lightModelLoc = glGetUniformLocation(lightCubeShader->ID, "model");

	// ONLY render physical light cubes if a light object exists in sceneObjects
	if (hasLightEntity) {
		for (const auto& obj : sceneObjects) {
			if (!obj.visible || obj.type != ObjectType::Light) continue;

			glm::mat4 lightModel = obj.transformMatrix;
			lightModel = glm::scale(lightModel, glm::vec3(0.2f));

			glUniformMatrix4fv(lightModelLoc, 1, GL_FALSE, glm::value_ptr(lightModel));
			glBindVertexArray(lightVAO);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		}
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}