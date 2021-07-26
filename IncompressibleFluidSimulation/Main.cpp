#include <iostream>

// utility
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <shader.h>

#include <tuple>
#include <windows.h>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "FluidBox.h"

using namespace std;

// settings
const int resolution = 300;
const double fps = 60;

// control settings
bool enableTracers = false;

// status vars
int SCR_HEIGHT;
int SCR_WIDTH;

enum ControlMode { NONE = 0, DIRECTIONAL = 1, MOUSE_SWIPE = 2, MOUSE_CLICK_BLOP = 3 };

struct MouseData {
	bool pressedL;
	bool pressedR;
	bool dragging;

	bool prevPressedL;
	bool prevPressedR;

	//glm::vec2 pressPos;
	//glm::vec2 releasePos;

	glm::vec2 currentPos;
	glm::vec2 lastPos;

	MouseData() {
		pressedL = false;
		pressedR = false;
		//pressPos = glm::vec2(0);
		//releasePos = glm::vec2(0);
		currentPos = glm::vec2(0);
		lastPos = glm::vec2(0);
	}

	void update() {
		prevPressedR = pressedR;
		prevPressedL = pressedL;
	}
};

struct FPSCounter {
	std::chrono::system_clock::time_point now;
	int fpsCount;
	int fpsCounter;

	int updateInterval = 60;

	int storedFPS;

	FPSCounter() {
		fpsCount = 0;
		fpsCounter = 0;
		storedFPS = 0;
	}

	void start() {
		now = std::chrono::system_clock::now();
	}

	void end() {
		//end of timer sleep and normalize the clock
		std::chrono::system_clock::time_point after = std::chrono::system_clock::now();
		std::chrono::microseconds difference(std::chrono::time_point_cast<std::chrono::microseconds>(after) - std::chrono::time_point_cast<std::chrono::microseconds>(now));

		int diffCount = difference.count();
		if (diffCount == 0) {
			diffCount = 1;
		}

		// apply fps to average
		fpsCount += 1;
		fpsCounter += 1000000 / diffCount;

		if (fpsCount % int(updateInterval) == 0) {
			storedFPS = fpsCounter / fpsCount;

			fpsCount = 0;
			fpsCounter = 0;
		}
	}

	void printFPS(bool sameLine = false) {
		if (sameLine) { std::cout << "\r"; }
		std::cout << "FPS: " << storedFPS;
		if (!sameLine) { std::cout << std::endl; }
	}
};

struct RenderObject {
	Shader shader;
	unsigned int VAO;
	unsigned int VBO;

	float* data = nullptr;

	RenderObject() {
		glGenBuffers(1, &VBO);
		glGenVertexArrays(1, &VAO);
	}

	void allocateMemory(int size) {
		if (data != nullptr) {
			delete[] data;
		}

		data = new float[size];
	}
};

//prototypes
// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// fluid modifiers
void addDirectionalFluid(FluidBox& fluid, int brushSize = 5, float densityInc = 20.0f, float velocityInc = 0.01f, glm::vec2 pos = glm::vec2(100.0f, 100.0f), glm::vec2 dir = glm::vec2(1.0f, 0.0f), glm::vec3 color = glm::vec3(255));
void addMouseSwipeFluid();
void addBlop(glm::vec2 pos, int radius, float densityInc, float velocityInc);
void addMouseClickBlop();

// functions
void processControls(GLFWwindow* window, FluidBox& fluid, ControlMode& controlMode);
glm::vec3 getColorSpect(float n, float m);

tuple<unsigned int, unsigned int> findWindowDims(float relativeScreenSize = 0.85, float aspectRatio = 1);
void updateData(FluidBox &fluidBox, float* data);
void updateBuffers(RenderObject* renderObject);
void updateForces(FluidBox& fluid);
void containTracers(FluidBox& fluid, int min, int max);
bool constrain(glm::vec2& vec, float min, float max);
void constrain(float &num, float min, float max);

// Control structs
MouseData mouse;

// global stuff
// main vars
GLFWwindow* window;
FluidBox* fluid;
RenderObject* renderFluid;

// control vars
ControlMode controlMode;
bool freeze;

int colorIndex;
int colorInc;
int colorSpectSize;

void setup() {
	fluid = new FluidBox(resolution, 0.1f, 0.0000001f, 0.4f);
	controlMode = ControlMode::MOUSE_SWIPE;
	freeze = false;

	colorIndex = 0;
	colorInc = 1;
	colorSpectSize = 5;

	mouse = MouseData();

	// init graphics stuff
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	std::tie(SCR_WIDTH, SCR_HEIGHT) = findWindowDims(0.7f, 1.0f);

	//SCR_WIDTH = 300;
	//SCR_HEIGHT = 300;

	// glfw window creation
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Fluid Sim", NULL, NULL);

	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
	}
	glfwMakeContextCurrent(window);

	// set callbacks
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	// setup fluid render stuff
	renderFluid = new RenderObject();
	renderFluid->shader = Shader("resources/shaders/point_render.vs", "resources/shaders/point_render.fs", "resources/shaders/point_render.gs");
	renderFluid->allocateMemory((resolution * resolution - 4 - resolution * 4) * (2 + 3));

	updateData(*fluid, renderFluid->data);
	updateBuffers(renderFluid);
}

int main() {
	setup();

	FPSCounter timer = FPSCounter();

	std::cout << "entering main loop" << std::endl;

	while (!glfwWindowShouldClose(window)) {
		timer.start();

		processControls(window, *fluid, controlMode);

		// update frame
		if (!freeze) {
			fluid->update();
			fluid->fadeDensity(0.05f, 0, 255);
		}


		updateData(*fluid, renderFluid->data);
		updateBuffers(renderFluid);

		//draw
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderFluid->shader.use();
		renderFluid->shader.setFloat("sizeX", 2.0f / resolution);
		renderFluid->shader.setFloat("sizeY", 2.0f / resolution);

		glBindVertexArray(renderFluid->VAO);
		glDrawArrays(GL_POINTS, 0, resolution * resolution);

		mouse.update();

		// update view
		glfwSwapBuffers(window);
		glfwPollEvents();

		timer.end();
		timer.printFPS(true);
	}

	glfwTerminate();
	delete[] renderFluid->data;

	return 0;
}

void processControls(GLFWwindow* window, FluidBox& fluid, ControlMode& controlMode) {
	// process mouse input
	if (mouse.pressedL) {
		if (controlMode == ControlMode::MOUSE_SWIPE) {
			addMouseSwipeFluid();
			glm::vec2 scaling = glm::vec2(float(resolution) / SCR_WIDTH, float(resolution) / SCR_HEIGHT);
		}
		if (controlMode == ControlMode::MOUSE_CLICK_BLOP) {
			addMouseClickBlop();
		}
	}
	else if (mouse.prevPressedL){
		//colorIndex = (colorIndex + 1) % (sizeof(colorList) / sizeof(glm::vec3));
		colorIndex = (colorIndex + colorInc) % colorSpectSize;
	}

	// clear screen controls
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
		fluid.clear();
	}

	// process key input
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		
	}

	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		
	}

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	// default values
	freeze = false;
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		switch (controlMode) {
		case 0:
			break;
		case 1:
			addDirectionalFluid(fluid);
			break;
		case 2:
			freeze = true;
			break;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
		switch (controlMode) {
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		}
	}
}

void addMouseClickBlop() {
	glm::vec2 scaling = glm::vec2(float(resolution) / SCR_WIDTH, float(resolution) / SCR_HEIGHT);
	addBlop(mouse.currentPos * scaling, 1, 20.0f, 0.01f * 0.15f);
}

void addBlop(glm::vec2 pos, int radius, float densityInc, float velocityInc) {
	constrain(densityInc, 0, 50);
	//constrain(velocityInc, 0, 0.15);

	for (int y = -radius; y < radius; y++) {
		for (int x = -radius; x < radius; x++) {
			// make a circle for the density and velocity to be added
			if (glm::length(glm::vec2(x, y)) <= radius) {
				glm::vec2 cpos = glm::vec2(pos.x + x, pos.y + y);
				glm::vec2 dir = glm::normalize(glm::vec2(x, y));

				std::cout << glm::to_string(cpos) << std::endl;

				// skip if the pos is out of bounds
				if (!constrain(cpos, 1, (*fluid).size - 2)) {
					(*fluid).addDensity(cpos, densityInc * (float(std::rand()) / INT_MAX + 0.5f));
					(*fluid).addVelocity(cpos, velocityInc * dir);
					std::cout << glm::to_string(velocityInc * dir) << " " << densityInc * (float(std::rand()) / INT_MAX + 0.5f) << std::endl;
				}
			}
		}
	}
}

void addMouseSwipeFluid() {
	glm::vec2 scaling = glm::vec2(float(resolution) / SCR_WIDTH, float(resolution) / SCR_HEIGHT);
	float mouseDiff = glm::length((mouse.currentPos - mouse.lastPos) * scaling);
	float densityInc = 0.2f * (mouseDiff / 4.0f);
	addDirectionalFluid(*fluid, 10, densityInc, 0.025f * (mouseDiff / 10.0f), mouse.currentPos * scaling, (mouse.currentPos - mouse.lastPos) * scaling, getColorSpect(colorIndex, colorSpectSize));
}

void addDirectionalFluid(FluidBox& fluid, int brushSize, float densityInc, float velocityInc, glm::vec2 pos, glm::vec2 dir, glm::vec3 color){
	constrain(densityInc, 0, 50);
	constrain(velocityInc, 0, 0.15);

	dir = glm::normalize(dir);

	for (int y = -brushSize; y < brushSize; y++) {
		for (int x = -brushSize; x < brushSize; x++) {
			// make a circle for the density and velocity to be added
			if (glm::length(glm::vec2(x, y)) <= brushSize) {
				glm::vec2 cpos = glm::vec2(pos.x + x, pos.y + y);
				// skip if the pos is out of bounds
				if (!constrain(cpos, 1, fluid.size - 2)) {
					fluid.addDensity(cpos, densityInc * (float(std::rand()) / INT_MAX + 0.5f), color);
					fluid.addVelocity(cpos, velocityInc * dir);

					if (enableTracers) {
						fluid.addTracer(cpos, getColorSpect(colorIndex, colorSpectSize));
					}
				}
			}
		}
	}
}

void containTracers(FluidBox& fluid, int min, int max) {
	for (int y = 0; y < fluid.size; y++) {
		for (int x = 0; x < fluid.size; x++) {
			for (int i = 0; i < fluid.density.size(); i++) {
				if (fluid.density[i][y][x] < min) {
					fluid.density[i][y][x] = min;
				}
				if (fluid.density[i][y][x] > max) {
					fluid.density[i][y][x] = max;
				}
			}
		}
	}
}

void updateData(FluidBox &fluidBox, float* data) {
	int index = 0;
	vector<vector<Tracer*>> tracerMap;

	if (enableTracers) {
		tracerMap = fluid->generateTracerMap();
	}

	for (int y = 1; y < fluidBox.size-1; y++) {
		for (int x = 1; x < fluidBox.size-1; x++) {
			data[index] = float(x) / fluidBox.size;
			data[index + 1] = float(y) / fluidBox.size;

			//data[index + 2] = 0;
			//data[index + 3] = 0;
			//data[index + 4] = 0;
			
			//float color = (fluidBox.density[y][x] / 255.0f);
			//data[index + 2] = color;
			//data[index + 3] = color;
			//data[index + 4] = color;

			glm::vec3 color = fluidBox.getColorAtPos(glm::vec2(x,y));
			data[index + 2] = color.x;
			data[index + 3] = color.y;
			data[index + 4] = color.z;

			if (enableTracers && tracerMap[y][x] != nullptr) {
				data[index + 2] *= tracerMap[y][x]->color.x;
				data[index + 3] *= tracerMap[y][x]->color.y;
				data[index + 4] *= tracerMap[y][x]->color.z;
			}

			//float alpha = fluidBox.density[y][x] / 255.0f;
			//glm::vec3 color = alpha * fluidBox.getColorAtPos(glm::vec2(x, y));
			//data[index + 2] = color.x/255;
			//data[index + 3] = color.y/255;
			//data[index + 4] = color.z/255;

			//data[index + 2] = fluidBox.density[y][x] / 255.0f;
			//data[index + 3] = 1000 * abs(fluidBox.velocity->getXList()[y][x]) / 255.0f;
			//data[index + 4] = 1000 * abs(fluidBox.velocity->getYList()[y][x]) / 255.0f;

			//data[index + 2] = std::fmod(fluidBox.density[y][x] + 50, 200.0f) / 255.0f;
			//data[index + 3] = 200 / 255.0f;
			//data[index + 4] = fluidBox.density[y][x] / 255.0f;
			index += 5;

			//std::cout << data[index-5] << " " << data[index - 4] << " " << data[index - 3] << " " << data[index - 2] << " " << data[index - 1] << std::endl;
		}
	}
}

void updateBuffers(RenderObject* renderObject) {
	glBindVertexArray(renderObject->VAO);

	glBindBuffer(GL_ARRAY_BUFFER, renderObject->VBO);
	glBufferData(GL_ARRAY_BUFFER, ((resolution * resolution - 4 - resolution * 4) * (2 + 3))*sizeof(float), renderObject->data, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));

	glBindVertexArray(0);
}

// sinusoidal color function
glm::vec3 getColorSpect(float n, float m) {
	float PI = 3.14159265358979f;

	float a = 5 * PI * n / (3 * m) + PI / 2;

	float r = sin(a) * 192 + 128;
	r = max(0, min(255, r));
	float g = sin(a - 2 * PI / 3) * 192 + 128;
	g = max(0, min(255, g));
	float b = sin(a - 4 * PI / 3) * 192 + 128;
	b = max(0, min(255, b));

	return glm::vec3(r, g, b);
}

// finds the optimal dimensions for the window
tuple<unsigned int, unsigned int> findWindowDims(float relativeScreenSize, float aspectRatio) {
	// set window size to max while also maintaining size ratio
	RECT rect;
	GetClientRect(GetDesktopWindow(), &rect);

	unsigned int SCR_WIDTH = (rect.right - rect.left) * relativeScreenSize;
	unsigned int SCR_HEIGHT = (rect.bottom - rect.top) * relativeScreenSize;

	return tuple<unsigned int, unsigned int>{SCR_HEIGHT, SCR_HEIGHT};
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// std::cout << "Failed to create GLFW window" << std::endl;
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on some displays

	SCR_WIDTH = width;
	SCR_HEIGHT = height;

	glViewport(0, 0, width, height);
}

// clicking
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		mouse.pressedL = true;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		mouse.pressedL = false;
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		mouse.pressedR = true;
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		mouse.pressedR = false;
	}
}

// mouse movement
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	int winxpos, winypos;
	glfwGetWindowPos(window, &winxpos, &winypos);

	xpos = xpos;
	ypos = -ypos + SCR_HEIGHT;

	mouse.lastPos = mouse.currentPos;
	mouse.currentPos = glm::vec2(xpos, ypos);
}