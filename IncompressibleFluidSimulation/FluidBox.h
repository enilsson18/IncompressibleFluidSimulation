#pragma once

// basic
#include <iostream> 

// graphics libs for calculations
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <shader.h>
#include "FBO.h"
#include "Quad.h"

// vector and math lib
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <vector>
#include <algorithm>

struct DynamicVector {
	// (access dim, y, x)
	std::vector<std::vector<std::vector<float>>> vector;

	DynamicVector(int x_size, int y_size, float default_value = 0) {
		vector = std::vector<std::vector<std::vector<float>>>(2, std::vector<std::vector<float>>(y_size, std::vector<float>(x_size, default_value)));
	}

	std::vector<std::vector<float>> &getXList() {
		return vector[0];
	}

	std::vector<std::vector<float>> &getYList() {
		return vector[1];
	}

	glm::vec2 getVec(int x, int y) {
		return glm::vec2(vector[0][y][x], vector[1][y][x]);
	}

	glm::vec2 getVec(glm::vec2 vec) {
		int x = (int) vec.x;
		int y = (int) vec.y;

		return getVec(x, y);
	}
};

struct Tracer {
	glm::vec2 pos;
	glm::vec3 color;

	Tracer(glm::vec2 pos, glm::vec3 color) {
		this->pos = pos;
		this->color = color;
	}
};

class FluidBox {
private:
	// absolute paths for the shaders
	const char* basicVertexShader = "resources/shaders/render_quad.vs";
	const char* basicFragmentShader = "resources/shaders/render_quad.fs";
	const char* boundFragmentShader = "resources/shaders/enforce_bounds.fs";
	const char* jacobiFragmentShader = "resources/shaders/jacobi.fs";
	const char* divergenceFragmentShader = "resources/shaders/divergence.fs";
	const char* gradSubFragmentShader = "resources/shaders/grad_sub.fs";
	const char* advectFragmentShader = "resources/shaders/advect.fs";
	const char* addDensityFragmentShader = "resources/shaders/add_density.fs";

public:
	// settings
	int size;
	float dt;
	float diff;
	float visc;
	float divIter;

	float texScale;

	int updateCount = 0;

	bool velocityFrozen;

	// shaders for processing
	Shader* copyShader;
	Shader* boundShader;
	Shader* jacobiShader;
	Shader* divShader;
	Shader* gradShader;
	Shader* advectShader;
	Shader* addShader;

	// Runtime vars
	// Render box data
	float interior[16];
	float exterior[4][16];

	// Pressure field
	FBO* pressure;
	// density (one is the previous stored value and the other is the current value)
	FBO* density;
	// velocity
	FBO* velocity;
	// temporary storage for the divergence mapping
	FBO* div;

	// Color Tracers (Each array contains the rgb float values "0-255")
	std::vector<Tracer> tracers;

	FluidBox(int size, float diffusion, float viscosity, float dt);

	void update();

	void resetSize(int size);

	void enforceBounds(FBO* x, float scale, bool useHalf = false);

	// applies diffusion on the input fbo (normally density or velocity)
	void diffuse(FBO* v);
	// updates the velocity based on the pressure gradient 
	// (input the velocity map, pressure map, and a temporary storage image of the same size)
	void project(FBO* v, FBO* p, FBO* d);
	// update the d image based on the velocity map input as v
	void advect(FBO* v, FBO* d);

	void updateTracers();

	void fadeDensity(float increment, float min, float max);
	
	// copy f2 into f1
	void copyTo(FBO* f1, FBO* f2);

	void addTracer(glm::vec2 pos, glm::vec3 color);
	// Enter the pos in terms of a coordinate grid with the size of the resolution
	// Amount is the multiplier for color
	// Color is in terms of rgb(0.0-255.0)
	// radius is how wide of a range (in terms of size) the density will be added
	void addDensity(glm::vec2 pos, float amount, glm::vec3 color = glm::vec3(1.0f), float radius = 0.1f);
	// Enter the pos in terms of a coordinate grid with the size of the resolution
	// Amount is the magnitude of the velocity to be added
	// radius is how wide of a range (in terms of size) the velocity will be added
	void addVelocity(glm::vec2 pos, glm::vec2 amount, float radius);

	void freezeVelocity();
	void unfreezeVelocity();
	bool getFreezeVelocity();

	// recalculates the interior and exterior vertices if the resolution ever changes
	void recalculateRenderBoxes();
	// renders the whole interior box
	void renderInterior();
	// renders the specified side of the exterior box
	// 0 = top, 1 = right, 2 = bottom, 3 = left
	void renderExterior(int i);

	// initializes the shader classes based on their corresponding file path
	void setupShaders();

	// clears the density and velocity map
	void clear();

	void setTexScale(float scale);

	glm::vec3 getColorAtPos(glm::vec2 pos);

	std::vector<Tracer>& getTracers();

	std::vector<std::vector<Tracer*>> generateTracerMap();
};