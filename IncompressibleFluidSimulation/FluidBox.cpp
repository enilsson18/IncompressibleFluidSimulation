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
void constrain(float &num, float min, float max);
bool constrain(glm::vec2& vec, float min, float max);

FluidBox::FluidBox(int size, int diffusion, int viscosity, float dt) {
	//setup(size, diffusion, viscosity, dt);
	this->size = size;
	this->diff = diffusion;
	this->visc = viscosity;
	this->dt = dt;

	this->divIter = 4;

	// init 2d arrays
	clear();
};

// the main update step
void FluidBox::update() {
	vector<vector<float>>& vPrevXList = velocityPrev->getXList();
	vector<vector<float>>& vPrevYList = velocityPrev->getYList();
	
	vector<vector<float>>& vXList = velocity->getXList();
	vector<vector<float>>& vYList = velocity->getYList();

	diffuse(vPrevXList, vXList, 1);
	diffuse(vPrevYList, vYList, 2);

	project(vPrevXList, vPrevYList, vXList, vYList);

	advect(1, vPrevXList, vPrevYList, vXList, vPrevXList);
	advect(2, vPrevXList, vPrevYList, vYList, vPrevYList);

	project(vXList, vYList, vPrevXList, vPrevYList);

	// applys advection for each color channel
	for (int i = 0; i < 3; i++) {
		diffuse(prevDensity[i], density[i], 0);
		advect(0, vXList, vYList, density[i], prevDensity[i]);
	}

	updateTracers();

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

void FluidBox::advect(int b, std::vector<std::vector<float>> &vx, std::vector<std::vector<float>> &vy, std::vector<std::vector<float>> &d, std::vector<std::vector<float>> &d0) {
	float i0, i1, j0, j1;

	float dtx = dt * (size - 2);
	float dty = dt * (size - 2);

	float s0, s1, t0, t1;

	float Nfloat = size;

	for (int j = 1; j < size - 1; j++) {
		for (int i = 1; i < size - 1; i++) {
			calcUpstreamCoords(Nfloat, vx[j][i], vy[j][i], dtx, dty, i, j, i0, i1, j0, j1, s0, s1, t0, t1);

			int i0i = int(i0);
			int i1i = int(i1);
			int j0i = int(j0);
			int j1i = int(j1);

			constrain(i0i, 0, size - 1);
			constrain(i1i, 0, size - 1);
			constrain(j0i, 0, size - 1);
			constrain(j1i, 0, size - 1);

			d[j][i] =
				s0 * (t0 * d0[j0i][i0i] + t1 * d0[j1i][i0i]) +
				s1 * (t0 * d0[j0i][i1i] + t1 * d0[j1i][i1i]);
		}
	}

	enforceBounds(d, b);
}

void FluidBox::updateTracers() {
	float i0, i1, j0, j1;

	float dtx = dt * (size - 2);
	float dty = dt * (size - 2);

	float s0, s1, t0, t1;

	float Nfloat = size;

	for (int i = 0; i < tracers.size(); i++) {
		calcUpstreamCoords(Nfloat, velocity->getVec(tracers[i].pos).x, velocity->getVec(tracers[i].pos).y, dtx, dty, tracers[i].pos.x, tracers[i].pos.y, i0, i1, j0, j1, s0, s1, t0, t1);

		tracers[i].pos +=
			s0 * (t0 * (tracers[i].pos - glm::vec2(i0, j0)) + t1 * (tracers[i].pos - glm::vec2(i0, j1))) +
			s1 * (t0 * (tracers[i].pos - glm::vec2(i1, j0)) + t1 * (tracers[i].pos - glm::vec2(i1, j1))) - glm::vec2(0.5f);

		constrain(tracers[i].pos, 0, size - 1);
	}
}

void FluidBox::calcUpstreamCoords(float Nfloat, float vx, float vy, float dtx, float dty, int i, int j, float &i0, float &i1, float &j0, float &j1, float &s0, float &s1, float &t0, float &t1) {
	float tmp1, tmp2, x, y;

	tmp1 = dtx * vx;
	tmp2 = dty * vy;
	x = i - tmp1;
	y = j - tmp2;

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
}

void FluidBox::addTracer(glm::vec2 pos, glm::vec3 color) {
	if (constrain(pos, 0, size - 1)) {
		return;
	}

	tracers.push_back(Tracer(pos, color));
}

void FluidBox::addDensity(glm::vec2 pos, float amount, glm::vec3 color) {
	if (constrain(pos, 0, size - 1)) {
		return;
	}

	density[0][pos.y][pos.x] += amount * color.x / 255.0f;
	density[1][pos.y][pos.x] += amount * color.y / 255.0f;
	density[2][pos.y][pos.x] += amount * color.z / 255.0f;
}

void FluidBox::addVelocity(glm::vec2 pos, glm::vec2 amount) {
	if (constrain(pos, 1, size - 2)) {
		return;
	}

	velocity->getXList()[pos.y][pos.x] += amount.x;
	velocity->getYList()[pos.y][pos.x] += amount.y;
}

void FluidBox::clear() {
	this->prevDensity = vector<vector<vector<float>>>(3, vector<vector<float>>(size, vector<float>(size, 0)));
	this->density = vector<vector<vector<float>>>(3, vector<vector<float>>(size, vector<float>(size, 0)));
	this->tracers = vector<Tracer>();
	this->velocityPrev = new DynamicVector(size, size);
	this->velocity = new DynamicVector(size, size);
}

void FluidBox::fadeDensity(float increment, float min, float max) {
	int checkInterval = size / 60;
	float densityMultiplier = 10.0f / 255.0f;

	float avgDensity = 0;
	for (int y = 0; y < size; y += checkInterval) {
		for (int x = 0; x < size; x += checkInterval) {
			for (int i = 0; i < density.size(); i++) {
				avgDensity += density[i][y][x];
			}
		}
	}
	avgDensity /= 3 * (size / checkInterval)*(size / checkInterval);

	float densityIncrement = increment * (avgDensity * densityMultiplier);

	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			for (int i = 0; i < density.size(); i++) {
				density[i][y][x] -= densityIncrement;
				constrain(density[i][y][x], min, max);
			}
		}
	}
}

glm::vec3 FluidBox::getColorAtPos(glm::vec2 pos) {
	glm::vec3 output = glm::vec3(1);

	output.x = density[0][pos.y][pos.x];
	output.y = density[1][pos.y][pos.x];
	output.z = density[2][pos.y][pos.x];
	
	return output;
}

std::vector<Tracer>& FluidBox::getTracers() {
	return tracers;
}

vector<vector<Tracer*>> FluidBox::generateTracerMap() {
	vector<vector<Tracer*>> map = vector<vector<Tracer*>>(size, vector<Tracer*>(size, nullptr));

	for (int i = 0; i < tracers.size(); i++) {
		map[tracers[i].pos.y][tracers[i].pos.x] = &tracers[i];
	}

	return map;
}

// search for y row and then x if not found then return nullptr
// min is inclusive and max is exclusive
Tracer& binaryTracerSearch(vector<Tracer>& tracers, int min, int max, glm::vec2 targetPos) {
	int mid = (max - min) / 2 + min;
	int left = min + (mid - min) / 2;
	int right = min + 3 * (mid - min) / 2;

	// if ys are found then start searching xs
	if (tracers[mid].pos.y == targetPos.y) {
		if (tracers[left].pos.x == targetPos.x) {
			return tracers[left];
		}
		else if (tracers[left].pos.x < targetPos.x) {
			return binaryTracerSearch(tracers, left, mid, targetPos);
		}
		else {
			return binaryTracerSearch(tracers, min, left, targetPos);
		}
	}
	if (tracers[mid].pos.y == targetPos.y) {
		if (tracers[left].pos.x == targetPos.x) {
			return tracers[right];
		}
		else if (tracers[right].pos.x < targetPos.x) {
			return binaryTracerSearch(tracers, right, max, targetPos);
		}
		else {
			return binaryTracerSearch(tracers, mid, right, targetPos);
		}
	}
	// searching for y value
	else if (tracers[mid].pos.y < targetPos.y) {
		return binaryTracerSearch(tracers, mid, max, targetPos);
	}
	else {
		return binaryTracerSearch(tracers, min, mid, targetPos);
	}
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

bool constrain(glm::vec2& vec, float min, float max) {
	bool toReturn = false;

	if (vec.x < min) {
		vec.x = min;
		toReturn = true;
	}
	if (vec.y < min) {
		vec.y = min;
		toReturn = true;
	}
	if (vec.x > max) {
		vec.x = max;
		toReturn = true;
	}
	if (vec.y > max) {
		vec.y = max;
		toReturn = true;
	}

	return toReturn;
}