#pragma once

// basic
#include <iostream> 

// graphics libs for calculations
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <shader.h>
#include "FBO.h"

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
const char* diffuseFragmentShader = "resources/shaders/diffuse.fs";
const char* projectFragmentShader = "resources/shaders/project.fs";
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
	Shader* diffuseShader;
	Shader* projectShader;
	Shader* advectShader;

	// runtime vars
	// density (one is the previous stored value and the other is the current value)
	FBO* prevDensity;
	FBO* density;

	// Color Tracers (Each array contains the rgb float values "0-255")
	std::vector<Tracer> tracers;

	// velocity
	FBO* velocityPrev;
	FBO* velocity;

	FluidBox(int size, float diffusion, float viscosity, float dt);

	void update();

	void resetSize(int size);

	void enforceBounds(std::vector<std::vector<float>> &v, int dim = 1);
	void removeDivergence(std::vector<std::vector<float>> &v, std::vector<std::vector<float>> &vPrev, float a, float c, int b);

	void diffuse(std::vector<std::vector<float>> &v, std::vector<std::vector<float>> &vPrev, int b);
	void project(std::vector<std::vector<float>> &vx, std::vector<std::vector<float>> &vy, std::vector<std::vector<float>> &p, std::vector<std::vector<float>> &div);
	void advect(int b, std::vector<std::vector<float>> &vx, std::vector<std::vector<float>> &vy, std::vector<std::vector<float>> &d, std::vector<std::vector<float>> &d0);

	void updateTracers();

	void calcUpstreamCoords(float Nfloat, float vx, float vy, float dtx, float dty, int i, int j, float &i0, float &i1, float &j0, float &j1, float &s0, float &s1, float &t0, float &t1);

	void fadeDensity(float increment, float min, float max);

	void addTracer(glm::vec2 pos, glm::vec3 color);
	void addDensity(glm::vec2 pos, float amount, glm::vec3 color = glm::vec3(1.0f));
	void addVelocity(glm::vec2 pos, glm::vec2 amount);

	void freezeVelocity();
	void unfreezeVelocity();
	bool getFreezeVelocity();

	void setupShaders();

	void clear();

	glm::vec3 getColorAtPos(glm::vec2 pos);

	std::vector<Tracer>& getTracers();

	std::vector<std::vector<Tracer*>> generateTracerMap();
};