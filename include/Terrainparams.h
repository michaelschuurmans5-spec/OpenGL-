#pragma once


struct TerrainParams {
	float size = 20.0f; // Width depth of terrain 
	int resolution = 64;  // vertices per side 


	float hillScale = 1.0f;  // broad gentle rolling elevation 
	float mountainScale = 0.0f;  // sharp tall ridged peaks
	float valleyScale = 0.0f;  // broad depressions cut into the base terrain
	float holeScale = 0.0f;   // sparse localized pits/craters
	float rockScale = 0.0f;   // high-frequency surface roughness

	int seed = 1337;

	bool erosionEnabled = true;
	int erosionDroplets = 30000;

};