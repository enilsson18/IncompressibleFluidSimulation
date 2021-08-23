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

	recalculateRenderBoxes();

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
		enforceBounds(velocity, -1.0f);

		//advect(1, vPrevXList, vPrevYList, vXList, vPrevXList);
		//advect(2, vPrevXList, vPrevYList, vYList, vPrevYList);
		advect(velocity, velocity);
		enforceBounds(velocity, -1.0f);

		//project(vXList, vYList, vPrevXList, vPrevYList);
		project(velocity, pressure, div);
	}

	//diffuse(prevDensity[i], density[i], 0);
	//advect(0, vXList, vYList, density[i], prevDensity[i]);
	diffuse(density);
	advect(velocity, density);

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

	x->unbind();
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
	d->clear(); // reset image to blank state
	divShader->use();

	v->useTex();
	divShader->setFloat("rdx", 1.0f / size);

	renderInterior();

	// run jacobi shader on pressure map with the new divergence
	p->bind();
	p->clear(); // reset image to blank state

	for (int i = 0; i < divIter; i++) {
		jacobiShader->use();

		p->useTex(0);
		d->useTex(1);
		jacobiShader->setFloat("rdx", 1.0f / size);
		jacobiShader->setFloat("a", 1);
		jacobiShader->setFloat("recip", 4);

		renderInterior();
	}

	// contain the pressure
	enforceBounds(p, 1.0f);

	// apply gradient subtraction to the velocity map
	v->bind();
	gradShader->use();

	v->useTex(0);
	p->useTex(1);
	gradShader->setFloat("rdx", 1.0f / size);

	renderInterior();

	v->unbind();

	// enforce the bounds on the velocity map
	enforceBounds(v, -1.0f);
}

void FluidBox::advect(FBO* v, FBO* d) {
	// run advect shader
	d->bind();
	advectShader->use();

	v->useTex(0);
	d->useTex(1);

	advectShader->setFloat("rdx", 1.0f / size);
	advectShader->setFloat("dt", dt);

	d->unbind();
}

void FluidBox::updateTracers() {
	
}

void FluidBox::addTracer(glm::vec2 pos, glm::vec3 color) {
	if (constrain(pos, 0, size - 1)) {
		return;
	}

	tracers.push_back(Tracer(pos, color));
}

void FluidBox::addDensity(glm::vec2 pos, float amount, glm::vec3 color, float radius) {
	if (constrain(pos, 0, size - 1)) {
		return;
	}

	std::cout << "density added: " << glm::to_string(amount * color) << std::endl;

	div->bind();
	div->clear();
	addShader->use();

	density->useTex();
	addShader->setVec2("point", pos * (1.0f / size));
	addShader->setVec3("density", color * amount * (1.0f / 255));
	addShader->setFloat("radius", radius * (1.0f / size));

	Quad::render();

	density->bind();
	copyShader->use();
	div->useTex();

	Quad::render();

	density->unbind();
}

void FluidBox::addVelocity(glm::vec2 pos, glm::vec2 amount, float radius) {
	if (constrain(pos, 1, size - 2)) {
		return;
	}

	velocity->bind();
	addShader->use();

	velocity->useTex();
	addShader->setVec2("point", pos * (1.0f / size));
	addShader->setVec3("density", glm::vec3(pos.x, pos.y, 0.0f));
	addShader->setFloat("radius", radius * (1.0f / size));

	Quad::render();

	velocity->unbind();
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
	// since the frame we are rendering to is 2 units wide to form the 2d square of -1 to 1,
	// we need to scale the texel offset by 2
	float texPos = 1 - texel;
	float pos = 1 - 2 * texel;

	float newInterior[] = {
		// positions // textures
		-pos,  pos, texel, texPos,
		-pos, -pos, texel, texel,
		 pos,  pos, texPos, texPos,
		 pos, -pos, texPos, texel
	};

	for (int i = 0; i < 16; i ++) {
		interior[i] = newInterior[i];
	}

	// Set exterior moving in the order top, right, bottom, left.
	// Each row includes both corners for processing.
	glm::vec2 points[] = {
		// top left
		glm::vec2(-1, 1),
		// bottom left
		glm::vec2(-1, pos),
		// top right
		glm::vec2(1, 1),
		// bottom right
		glm::vec2(1, pos),
	};

	glm::vec2 texPoints[]{
		// top left
		glm::vec2(0, 1),
		// bottom left
		glm::vec2(0, texPos),
		// top right
		glm::vec2(1, 1),
		// bottom right
		glm::vec2(1, texPos),
	};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			points[j] = rotateND(points[j]);
			exterior[i][j * 4 + 0] = points[j].x;
			exterior[i][j * 4 + 1] = points[j].y;
			exterior[i][j * 4 + 2] = texPoints[j].x;
			exterior[i][j * 4 + 3] = texPoints[j].y;
		}
	}
}

void FluidBox::renderInterior()
{
	//Quad::customRender(interior);
}

void FluidBox::renderExterior(int i)
{
	//Quad::customRender(exterior[i]);
}

void FluidBox::setupShaders()
{
	copyShader = new Shader(basicVertexShader, basicFragmentShader);
	jacobiShader = new Shader(basicVertexShader, jacobiFragmentShader);
	boundShader = new Shader(basicVertexShader, boundFragmentShader);
	divShader = new Shader(basicVertexShader, divergenceFragmentShader);
	gradShader = new Shader(basicVertexShader, gradSubFragmentShader);
	advectShader = new Shader(basicVertexShader, advectFragmentShader);
	addShader = new Shader(basicVertexShader, addDensityFragmentShader);
}

void FluidBox::clear() {
	this->pressure = new FBO(size, size);
	this->density = new FBO(size, size);
	this->tracers = vector<Tracer>();
	this->velocity = new FBO(size, size);
	this->div = new FBO(size, size);
}

void FluidBox::fadeDensity(float increment, float min, float max) {

}

glm::vec3 FluidBox::getColorAtPos(glm::vec2 pos) {
	glm::vec3 output = glm::vec3(1);

	//output.x = density[0][pos.y][pos.x];
	//output.y = density[1][pos.y][pos.x];
	//output.z = density[2][pos.y][pos.x];
	
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