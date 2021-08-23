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

// absolute paths for the shaders
const char* basicVertexShader = "resources/shaders/render_quad.vs";
const char* boundFragmentShader = "resources/shaders/enforce_bounds.fs";
const char* jacobiFragmentShader = "resources/shaders/jacobi.fs";
const char* divergenceFragmentShader = "resources/shaders/divergence.fs";
const char* gradSubFragmentShader = "resources/shaders/grad_sub.fs";
const char* advectFragmentShader = "resources/shaders/advect.fs";

class FluidBox {
public:
	// settings
	int size;
	float dt;
	float diff;
	float visc;
	float divIter;

	int updateCount = 0;

	bool velocityFrozen;

	// shaders for processing
	Shader* boundShader;
	Shader* jacobiShader;
	Shader* divShader;
	Shader* gradShader;
	Shader* advectShader;

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

	void enforceBounds(FBO* x, float scale);

	void diffuse(FBO* v);
	void project(FBO* v, FBO* p, FBO* d);
	void advect(FBO* v, FBO* d);

	void updateTracers();

	void calcUpstreamCoords(float Nfloat, float vx, float vy, float dtx, float dty, int i, int j, float &i0, float &i1, float &j0, float &j1, float &s0, float &s1, float &t0, float &t1);

	void fadeDensity(float increment, float min, float max);

	void addTracer(glm::vec2 pos, glm::vec3 color);
	void addDensity(glm::vec2 pos, float amount, glm::vec3 color = glm::vec3(1.0f));
	void addVelocity(glm::vec2 pos, glm::vec2 amount);

	void freezeVelocity();
	void unfreezeVelocity();
	bool getFreezeVelocity();

	void recalculateRenderBoxes();
	void renderInterior();
	void renderExterior(int i);

	void setupShaders();

	void clear();

	glm::vec3 getColorAtPos(glm::vec2 pos);

	std::vector<Tracer>& getTracers();

	std::vector<std::vector<Tracer*>> generateTracerMap();
};