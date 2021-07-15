// basic
#include <iostream> 

#include "FluidBox.h"

// vector and math lib
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <vector>

using namespace std;

FluidBox::FluidBox(int size, int diffusion, int viscosity, float dt) {
	//setup(size, diffusion, viscosity, dt);
	this->size = size;
	this->diff = diffusion;
	this->visc = viscosity;
	this->dt = dt;

	// init 2d arrays
	this->densityPrev = vector<vector<float>>(size, vector<float>(size, 0));
	this->density = vector<vector<float>>(size, vector<float>(size, 0));
	this->velocityPrev = std::vector<std::vector<glm::vec2>>(size, vector<glm::vec2>(size, glm::vec2(0)));
	this->velocity = std::vector<std::vector<glm::vec2>>(size, vector<glm::vec2>(size, glm::vec2(0)));
};

void FluidBox::update() {
	enforceBounds();
}

void FluidBox::enforceBounds() {
	for (int x = 1; x < size - 1; x++) {
		if (velocity[1][x].y < 0) {
			velocity[0][x].y = -velocity[1][x].y;
		}
	}

	for (int y = 1; y < velocity.size()-1; y++) {
		if (velocity[y][1].x < 0) {
			velocity[y][0].x = -velocity[y][1].x;
		}

		if (velocity[y][size-2].x > 0) {
			velocity[y][size-1].x = -velocity[y][size-2].x;
		}
	}

	for (int x = 1; x < size - 1; x++) {
		if (velocity[size-2][x].y > 0) {
			velocity[size-1][x].y = -velocity[size-2][x].y;
		}
	}

	// top left
	velocity[0][0] = 0.5f * (velocity[0][1] + velocity[1][0]);

	// top right
	velocity[0][size-1] = 0.5f * (velocity[0][size-2] + velocity[1][size-1]);

	// bottom right
	velocity[size-1][size-1] = 0.5f * (velocity[size-1][size-2] + velocity[size-2][size-1]);

	//bottom left
	velocity[size-1][0] = 0.5f * (velocity[size-2][0] + velocity[size-1][1]);
}

void calculateDensity() {

}

void FluidBox::addDensity(glm::vec2 pos, float amount) {
	density[pos.y][pos.x] += amount;
}

void FluidBox::addVelocity(glm::vec2 pos, glm::vec2 amount) {
	velocity[pos.y][pos.x] += amount;
}