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

void constrain(int &num, int min, int max);

FluidBox::FluidBox(int size, int diffusion, int viscosity, float dt) {
	//setup(size, diffusion, viscosity, dt);
	this->size = size;
	this->diff = diffusion;
	this->visc = viscosity;
	this->dt = dt;

	this->divIter = 8;

	// init 2d arrays
	this->simDensity = vector<vector<float>>(size, vector<float>(size, 0));
	this->density = vector<vector<float>>(size, vector<float>(size, 0));
	this->velocityPrev = new DynamicVector(size, size);
	this->velocity = new DynamicVector(size, size);
};

// the main update step
void FluidBox::update() {
	diffuse(velocityPrev->getXList(), velocity->getXList(), 1);
	diffuse(velocityPrev->getYList(), velocity->getYList(), 2);

	project(velocityPrev->getXList(), velocityPrev->getYList(), velocity->getXList(), velocity->getYList());

	advect(1, velocity->getXList(), velocityPrev->getXList(), velocityPrev->getXList(), velocityPrev->getYList());
	advect(2, velocity->getYList(), velocityPrev->getYList(), velocityPrev->getXList(), velocityPrev->getYList());

	project(velocity->getXList(), velocity->getYList(), velocityPrev->getXList(), velocityPrev->getYList());

	diffuse(simDensity, density, 0);
	advect(0, density, simDensity, velocity->getXList(), velocity->getYList());

	//std::cout << velocity->getXList()[size / 2][size / 2] << std::endl;
	//std::cout << simDensity[size / 2][size / 2] << std::endl;
}

void FluidBox::enforceBounds(std::vector<std::vector<float>> &v, int dim) {
	// x
	if (dim == 1) {
		for (int y = 1; y < v.size() - 1; y++) {
			v[y][0] = -v[y][1];

			v[y][size - 1] = -v[y][size - 2];
		}
	}
	
	// y
	if (dim == 2) {
		for (int i = 1; i < size - 1; i++) {
			v[0][i] = -v[1][i];

			v[size - 1][i] = -v[size - 2][i];
		}
	}

	// top left
	v[0][0] = 0.5f * (v[0][1] + v[1][0]);

	// top right
	v[0][size-1] = 0.5f * (v[0][size-2] + v[1][size-1]);

	// bottom right
	v[size-1][size-1] = 0.5f * (v[size-1][size-2] + v[size-2][size-1]);

	//bottom left
	v[size-1][0] = 0.5f * (v[size-2][0] + v[size-1][1]);
}

// Accounts for divergence in the velocity vectors
void FluidBox::removeDivergence(std::vector<std::vector<float>> &v, std::vector<std::vector<float>> &vPrev, float a, float c, int b) {
	float cRecip = 1 / c;

	for (int i = 0; i < divIter; i++) {
		for (int y = 1; y < size - 1; y++) {
			for (int x = 1; x < size - 1; x++) {
				// remove divergence for each coord
				v[y][x] = (vPrev[y][x] +
					a * (
						v[y + 1][x] +
						v[y - 1][x] +
						v[y][x + 1] +
						v[y][x - 1]
						)
					) * cRecip;
			}
		}

		enforceBounds(v, b);
	}
}

void FluidBox::diffuse(std::vector<std::vector<float>> &v, std::vector<std::vector<float>> &vPrev, int b) {
	float a = dt * diff * (size - 2) * (size - 2);
	removeDivergence(v, vPrev, a, 1 + 4 * a, b);
}

void FluidBox::project(std::vector<std::vector<float>> &vx, std::vector<std::vector<float>> &vy, std::vector<std::vector<float>> &p, std::vector<std::vector<float>> &div) {
	for (int y = 1; y < size - 1; y++) {
		for (int x = 1; x < size - 1; x++) {
			div[y][x] = -0.5f*(
				  vx[y][x+1]
				- vx[y][x-1]
				+ vy[y+1][x]
				- vy[y-1][x]
				) / size;
			p[y][x] = 0;
		}
	}

	enforceBounds(p);
	enforceBounds(div);
	removeDivergence(p, div, 1, 4, 0);

	for (int y = 1; y < size - 1; y++) {
		for (int x = 1; x < size - 1; x++) {
			vx[y][x] -= 0.5f * (p[y][x+1] - p[y][x-1]) * size;
			vy[y][x] -= 0.5f * (p[y+1][x] - p[y-1][x]) * size;
		}
	}
	enforceBounds(vx, 1);
	enforceBounds(vy, 2);
}

void FluidBox::advect(int b, std::vector<std::vector<float>> &d, std::vector<std::vector<float>> &d0, std::vector<std::vector<float>> &vx, std::vector<std::vector<float>> &vy) {
	float i0, i1, j0, j1;

	float dtx = dt * (size - 2);
	float dty = dt * (size - 2);

	float s0, s1, t0, t1;
	float tmp1, tmp2, x, y;

	float Nfloat = size;
	float ifloat, jfloat;

	for (int j = 1, jfloat = 1; j < size - 1; j++, jfloat++) {
		for (int i = 1, ifloat = 1; i < size - 1; i++, ifloat++) {
			tmp1 = dtx * vx[j][i];
			tmp2 = dty * vy[j][i];
			x = ifloat - tmp1;
			y = jfloat - tmp2;

			if (x < 0.5f) x = 0.5f;
			if (x > Nfloat + 0.5f) x = Nfloat + 0.5f;
			i0 = floor(x);
			i1 = i0 + 1.0f;
			if (y < 0.5f) y = 0.5f;
			if (y > Nfloat + 0.5f) y = Nfloat + 0.5f;
			j0 = floor(y);
			j1 = j0 + 1.0f;

			s1 = x - i0;
			s0 = 1.0f - s1;
			t1 = y - j0;
			t0 = 1.0f - t1;

			int i0i = int(i0);
			int i1i = int(i1);
			int j0i = int(j0);
			int j1i = int(j1);

			//constrain(i0i, 1, size - 2);
			//constrain(i1i, 1, size - 2);
			//constrain(j0i, 1, size - 2);
			//constrain(j1i, 1, size - 2);

			// DOUBLE CHECK THIS!!!
			if (false && updateCount > 119) {
				std::cout << j << " " << i << " " << j0i << " " << i0i << " " << j1i << " " << i1i << std::endl;
				std::cout << j0 << " " << j1 << std::endl;
				std::cout << y << " " << floor(y) << std::endl;
				std::cout << jfloat << " " << tmp2 << " " << tmp1 << std::endl;
				std::cout << dty << " " << vy[j][i] << std::endl;
			}
			d[j][i] =
				s0 * (t0 * d0[j0i][i0i] + t1 * d0[j1i][i0i]) +
				s1 * (t0 * d0[j0i][i1i] + t1 * d0[j1i][i1i]);
		}
	}

	enforceBounds(d, b);

	updateCount += 1;
}

void constrain(int &num, int min, int max) {
	if (num < min) {
		num = min;
	}
	if (num > max) {
		num = max;
	}
}

void constrain(float &num, float min, float max) {
	if (num < min) {
		num = min;
	}
	if (num > max) {
		num = max;
	}
}

void FluidBox::addDensity(glm::vec2 pos, float amount) {
	density[pos.y][pos.x] += amount;
}

void FluidBox::addVelocity(glm::vec2 pos, glm::vec2 amount) {
	velocity->getXList()[pos.y][pos.x] += amount.x;
	velocity->getYList()[pos.y][pos.x] += amount.y;
}