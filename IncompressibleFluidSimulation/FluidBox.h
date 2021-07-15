// basic
#include <iostream> 

// vector and math lib
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <vector>

class FluidBox {
public:
	// settings
	int size;
	float dt;
	float diff;
	float visc;

	// runtime vars
	// density
	std::vector<std::vector<float>> densityPrev;
	std::vector<std::vector<float>> density;

	// velocity
	std::vector<std::vector<glm::vec2>> velocityPrev;
	std::vector<std::vector<glm::vec2>> velocity;

	FluidBox(int size, int diffusion, int viscosity, float dt);

	void update();

	void enforceBounds();
	void calculateDensity();

	void diffuse();
	void project();
	void advect();

	void addDensity(glm::vec2 pos, float amount);
	void addVelocity(glm::vec2 pos, glm::vec2 amount);
};