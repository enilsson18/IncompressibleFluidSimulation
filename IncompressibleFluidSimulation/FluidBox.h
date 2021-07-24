// basic
#include <iostream> 

// vector and math lib
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <vector>

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
};

struct Tracer {
	glm::vec2 pos;
	glm::vec3 color;
};

class FluidBox {
public:
	// settings
	int size;
	float dt;
	float diff;
	float visc;
	float divIter;

	int updateCount = 0;

	// runtime vars
	// density (one is a tracer and the other is for the simulation)
	std::vector<std::vector<float>> simDensity;
	std::vector<std::vector<float>> density;

	// Color Tracers (Each array contains the rgb float values "0-255")
	std::vector<Tracer> tracers;

	// velocity
	DynamicVector* velocityPrev;
	DynamicVector* velocity;

	FluidBox(int size, int diffusion, int viscosity, float dt);

	void update();

	void enforceBounds(std::vector<std::vector<float>> &v, int dim = 1);
	void removeDivergence(std::vector<std::vector<float>> &v, std::vector<std::vector<float>> &vPrev, float a, float c, int b);

	void diffuse(std::vector<std::vector<float>> &v, std::vector<std::vector<float>> &vPrev, int b);
	void project(std::vector<std::vector<float>> &vx, std::vector<std::vector<float>> &vy, std::vector<std::vector<float>> &p, std::vector<std::vector<float>> &div);
	void advect(int b, std::vector<std::vector<float>> &vx, std::vector<std::vector<float>> &vy, std::vector<std::vector<float>> *d = nullptr, std::vector<std::vector<float>> *d0 = nullptr, void(FluidBox::*addProc)(float, float, float, float, int, int, int, int) = nullptr);

	void updateTracerPos(float x0, float x, float y0, float y, int x0i, int xi, int y0i, int yi);
	void updateTracers();

	void fadeDensity(float increment, float min, float max);

	void addTracer(glm::vec2 pos, glm::vec2 color);
	void addDensity(glm::vec2 pos, float amount, glm::vec3 color = glm::vec3(1.0f));
	void addVelocity(glm::vec2 pos, glm::vec2 amount);

	glm::vec3 getColorAtPos(glm::vec2 pos);
};