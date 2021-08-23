#include "FluidBox.h"

using namespace std;

void constrain(int &num, int min, int max);
void constrain(float &num, float min, float max);
bool constrain(glm::vec2& vec, float min, float max);

FluidBox::FluidBox(int size, float diffusion, float viscosity, float dt) {
	//setup(size, diffusion, viscosity, dt);
	this->size = size;
	this->diff = diffusion;
	this->visc = viscosity;
	this->dt = dt;

	this->divIter = 25;

	velocityFrozen = false;

	setupShaders();

	// init arrays
	clear();
};

// the main update step
void FluidBox::update() {
	if (!velocityFrozen) {
		// diffuse both dimensions
		//diffuse(vPrevXList, vXList, 1);
		//diffuse(vPrevYList, vYList, 2);
		diffuse(velocity);

		//advect(1, vPrevXList, vPrevYList, vXList, vPrevXList);
		//advect(2, vPrevXList, vPrevYList, vYList, vPrevYList);
		advect(velocity, velocity);

		//project(vXList, vYList, vPrevXList, vPrevYList);
		project(velocity, pressure, div);
	}

	// applys advection for each color channel
	for (int i = 0; i < 3; i++) {
		//diffuse(prevDensity[i], density[i], 0);
		//advect(0, vXList, vYList, density[i], prevDensity[i]);
		diffuse(0, prevDensity, i, density, i);
		advect(0, velocity, 0, velocity, 1, density, i, prevDensity, i);
	}

	// updateTracers();
}


void FluidBox::resetSize(int size) {
	/*
	float scale = size/this->size;
	int sizeToFit = min(this->size, size);

	int prevSize = this->size;
	this->size = size;

	vector<vector<vector<float>>> tempPrevDensity = prevDensity;
	vector<vector<vector<float>>> tempDensity = density;
	vector<Tracer> tempTracers = tracers;
	DynamicVector tempVelocityPrev = (*velocityPrev);
	DynamicVector tempVelocity = (*velocity);

	// reset to size
	clear();

	// copy data over into new array
	// density
	for (int i = 0; i < 3; i++) {
		for (int y = 0; y < sizeToFit; y++) {
			for (int x = 0; x < sizeToFit; x++) {
				prevDensity[i][y][x] = tempPrevDensity[i][y][x];
				density[i][y][x] = tempDensity[i][y][x];
			}
		}
	}

	// velocity
	for (int i = 0; i < 2; i++) {
		for (int y = 0; y < sizeToFit; y++) {
			for (int x = 0; x < sizeToFit; x++) {
				velocityPrev->vector[i][y][x] = tempVelocityPrev.vector[i][y][x];
				velocity->vector[i][y][x] = tempVelocity.vector[i][y][x];
			}
		}
	}

	// tracers
	for (int i = 0; i < tempTracers.size(); i++) {
		glm::vec2 vec = tempTracers[i].pos;
		constrain(tempTracers[i].pos, 0, sizeToFit);

		addTracer(vec, tempTracers[i].color);
	}
	*/
}

void FluidBox::enforceBounds(FBO* x, float scale) {
	x->bind();

	float texel = 1.0f / size;

	glm::vec2 offsetList[4] = {
		glm::vec2(0, -1),
		glm::vec2(-1, 0),
		glm::vec2(0, 1),
		glm::vec2(1, 0)
	};

	for (int i = 0; i < 4; i++) {
		boundShader->use();
		x->useTex();
		boundShader->setFloat("scale", scale);
		boundShader->setVec2("offset", offsetList[i] * texel);

		renderExterior(i);
	}
}

void FluidBox::diffuse(FBO* v) {
	float a = dt * diff * (size - 2) * (size - 2);
	float recip = 1 / (1 + 4 * a);

	// run jacobi shader
	v->bind();
	jacobiShader->use();
	
	v->useTex(0);
	v->useTex(1);
	jacobiShader->setFloat("rdx", 1.0f / size);
	jacobiShader->setFloat("a", a);
	jacobiShader->setFloat("recip", a);

	Quad::render();

	v->unbind();
}

void FluidBox::project(FBO* v, FBO* p, FBO* d) {
	// run divergence shader
	d->bind();
	d->clear();
	divShader->use();

	v->useTex();
	divShader->setFloat("rdx", 1.0f / size);

	Quad::render();

	// enforce bounds on div map and pressure map
	enforceBounds(p, 1);
	enforceBounds(div, -1);

	// run jacobi shader on pressure map with the new divergence
	p->bind();
	p->clear();

	for (int i = 0; i < divIter; i++) {
		jacobiShader->use();

		p->useTex(0);
		d->useTex(1);
		jacobiShader->setFloat("rdx", 1.0f / size);
		jacobiShader->setFloat("a", 1);
		jacobiShader->setFloat("recip", 4);

		Quad::render();
	}

	// apply gradient subtraction to the velocity map
	v->bind();
	gradShader->use();

	v->useTex(0);
	p->useTex(1);
	gradShader->setFloat("rdx", 1.0f / size);

	Quad::render();

	v->unbind();

	// enforce the bounds on the velocity map
}

void FluidBox::advect(FBO* v, FBO* d) {
	// run advect shader
	d->bind();
	advectShader->use();

	v->useTex(0);
	d->useTex(1);

	advectShader->setFloat("rdx", 1.0f / size);
	advectShader->setFloat("dt", dt);

	Quad::render();

	d->unbind();
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

void FluidBox::freezeVelocity()
{
	velocityFrozen = true;
}

void FluidBox::unfreezeVelocity()
{
	velocityFrozen = false;
}

bool FluidBox::getFreezeVelocity()
{
	return velocityFrozen;
}

// rotates a point clockwise by 90 degrees about the origin
glm::vec2 rotateND(glm::vec2 point) {
	return glm::vec2(point.y, -point.x);
}

void FluidBox::recalculateRenderBoxes()
{
	float texel = 1.0f / size;

	// Set interior (Leave 1 texel border excluded)
	float pos = 1 - texel;

	float newInterior[] = {
		// positions
		-pos,  pos,
		-pos, -pos,
		 pos,  pos,
		 pos, -pos
	};

	for (int i = 0; i < 16; i += 4) {
		interior[i] = newInterior[i / 2];
		interior[i + 1] = newInterior[i / 2 + 1];
	}

	// Set exterior moving in the order top, right, bottom, left.
	// Each row includes both corners for processing.
	glm::vec2 points[] = {
		// top left
		glm::vec2(-1, 1),
		// bottom left
		glm::vec2(-1, 1 - texel),
		// top right
		glm::vec2(1, 1),
		// bottom right
		glm::vec2(1, 1 - texel),
	};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			points[j] = rotateND(points[j]);
			exterior[i][j * 4] = points[j].x;
			exterior[i][j * 4 + 1] = points[j].y;
		}
	}
}

void FluidBox::renderInterior()
{
	Quad::customRender(interior);
}

void FluidBox::renderExterior(int i)
{
	Quad::customRender(exterior[i]);
}

void FluidBox::setupShaders()
{
	jacobiShader = new Shader(basicVertexShader, jacobiFragmentShader);
	boundShader = new Shader(basicVertexShader, boundFragmentShader);
	divShader = new Shader(basicVertexShader, divergenceFragmentShader);
	gradShader = new Shader(basicVertexShader, gradSubFragmentShader);
	advectShader = new Shader(basicVertexShader, advectFragmentShader);
}

void FluidBox::clear() {
	this->pressure = new FBO(size, size);
	this->density = new FBO(size, size);
	this->tracers = vector<Tracer>();
	this->velocity = new FBO(size, size);
	this->div = new FBO(size, size);
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