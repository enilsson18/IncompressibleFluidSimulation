#include <iostream>
#include <thread>

// utility
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <shader.h>

#include <tuple>
#include <windows.h>
#include <future>
#include <thread>
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
bool enableColor = true;

string commandToRead;
std::atomic<bool> enteredCommand;

// status vars
int SCR_HEIGHT;
int SCR_WIDTH;

enum ControlMode { NONE = 0, DIRECTIONAL = 1, MOUSE_SWIPE = 2 };

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
void incrementColorIndex();
glm::vec2 getScalingVec();
glm::vec3 getColorSpect(float n, float m);

 // commands
string enterCommand();
std::vector<string> seperateStringBySpaces(string str);
void printProcessCommandResult(bool result);
bool processCommand(string command);

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

FPSCounter timer;

int colorIndex;
int colorInc;
int colorSpectSize;

glm::vec3 defaultColor;

glm::vec3 tracerColor;

void setup() {
	fluid = new FluidBox(resolution, 0.1f, 0.0000001f, 0.4f);
	controlMode = ControlMode::MOUSE_SWIPE;
	freeze = false;

	colorIndex = 0;
	colorInc = 1;
	colorSpectSize = 5;

	defaultColor = glm::vec3(255);

	tracerColor = glm::vec3(defaultColor);

	mouse = MouseData();

	commandToRead = "";
	enteredCommand.store(false);

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

void draw() {
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderFluid->shader.use();
	renderFluid->shader.setFloat("sizeX", 2.0f / resolution);
	renderFluid->shader.setFloat("sizeY", 2.0f / resolution);

	glBindVertexArray(renderFluid->VAO);
	glDrawArrays(GL_POINTS, 0, resolution * resolution);
}

void updateFrame(FPSCounter& timer) {
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
	draw();

	mouse.update();

	// update view
	glfwSwapBuffers(window);
	glfwPollEvents();

	timer.end();
	//timer.printFPS(true);
}

// store the command input and then signal the main thread that we are complete and can exit the program
string commandInputThread() {
	std::cout << "Enter a command: ";

	commandToRead = enterCommand();
	enteredCommand.store(true);
	return commandToRead;
}

int main() {
	setup();

	timer = FPSCounter();
	//frameLoopThread(timer);

	future<string> commandInput = std::async(std::launch::async, commandInputThread);

	while (!glfwWindowShouldClose(window)) {
		updateFrame(timer);

		// if a command is sent then process, reset the command flag, and restart the async operation
		if (enteredCommand.load()) {
			bool out = processCommand(commandToRead);
			printProcessCommandResult(out);

			// reset vals
			commandToRead = "";
			enteredCommand.store(false);
			commandInput = std::async(std::launch::async, commandInputThread);
		}
	}

	glfwTerminate();
	delete[] renderFluid->data;

	return 0;
}

void listAllCommands() {
	std::cout << "----- Commands -----" << std::endl;

	std::cout <<
		"help" << std::endl <<
		"clear" << std::endl <<
		"get fps" << std::endl <<
		"set tracers enabled" << std::endl <<
		"set tracers disabled" << std::endl <<
		"set colors enabled" << std::endl <<
		"set colors disabled" << std::endl;

	std::cout << "--------------------" << std::endl;
}

// control methods
bool processCommand(string command) {
	// process and desegment
	// follows these steps
	// 1. Seperate words based on spaces into a list.
	// 2. Iterate down the list into a decision tree.
	// 3. Run the proper identified command

	std::vector<string> list = seperateStringBySpaces(command);

	if (list.size() == 0) {
		return false;
	}
	
	// command list
	if (list[0] == "help") {
		listAllCommands();
		return true;
	}

	if (list[0] == "clear") {
		(*fluid).clear();
		return true;
	}
	
	if (list[0] == "set") {
		if (list.size() > 1) {
			if (list[1] == "tracers") {
				if (list.size() > 2) {
					if (list[2] == "enabled") {
						enableTracers = true;
						return true;
					}
					if (list[2] == "disabled") {
						enableTracers = false;
						return true;
					}
				}
			}

			if (list[1] == "colors") {
				if (list.size() > 2) {
					if (list[2] == "enabled") {
						enableColor = true;
						return true;
					}
					if (list[2] == "disabled") {
						enableColor = false;
						return true;
					}
				}
			}
		}
	}

	if (list[0] == "get") {
		if (list.size() > 1) {
			if (list[1] == "fps") {
				timer.printFPS();
				return true;
			}
		}
	}

	return false;
}

void printProcessCommandResult(bool result) {
	if (result) {
		//std::cout << "Command Executed Sucessfully" << std::endl;
	}
	else {
		std::cout << "Command Executed Unsucessfully (type help for a list of all valid commands)" << std::endl;
	}
	std::cout << std::endl;
}

void processControls(GLFWwindow* window, FluidBox& fluid, ControlMode& controlMode) {
	// process mouse input
	if (mouse.pressedL) {
		if (controlMode == ControlMode::MOUSE_SWIPE) {
			addMouseSwipeFluid();
		}
	}
	else if (mouse.prevPressedL){
		incrementColorIndex();
	}

	// clear screen controls
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
		fluid.clear();
	}

	// enter commands
	if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_PRESS) {
		string command = enterCommand();

		printProcessCommandResult(processCommand(command));
	}

	// process key input
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
}

void addMouseSwipeFluid() {
	// An arbitrary number used for scaling the addition of fluid
	float densityMultiplier = 0.05f;
	float velocityMultiplier = 0.0025f;
	glm::vec2 scaling = getScalingVec();

	// The distance between the mouse's current pos and its previous pos
	float mouseDiff = glm::length((mouse.currentPos - mouse.lastPos) * scaling);

	// solve for parameters of directional fluid
	int brushSize = 10;
	float densityInc = densityMultiplier * mouseDiff;
	float velocityInc = velocityMultiplier * mouseDiff;
	glm::vec2 position = mouse.currentPos * scaling;
	glm::vec2 direction = (mouse.currentPos - mouse.lastPos) * scaling;
	glm::vec3 color = getColorSpect(colorIndex, colorSpectSize);

	addDirectionalFluid(*fluid, brushSize, densityInc, velocityInc, position, direction, color);
}

void addDirectionalFluid(FluidBox& fluid, int brushSize, float densityInc, float velocityInc, glm::vec2 pos, glm::vec2 dir, glm::vec3 color){
	// create limits for the increments to prevent too much velocity being added (can make divergence unsolvable) and also limits the density
	constrain(densityInc, 0, 50);
	constrain(velocityInc, 0, 0.15f);

	dir = glm::normalize(dir);

	if (!enableColor) {
		color = glm::vec3(defaultColor);
	}

	for (int y = -brushSize; y < brushSize; y++) {
		for (int x = -brushSize; x < brushSize; x++) {
			// make a circle for the density and velocity to be added
			if (glm::length(glm::vec2(x, y)) <= brushSize) {
				glm::vec2 cpos = glm::vec2(pos.x + x, pos.y + y);
				// skip if the pos is out of bounds
				if (!constrain(cpos, 1, fluid.size - 2)) {
					fluid.addDensity(cpos, densityInc * (float(std::rand()) / INT_MAX + 0.5f), color);
					fluid.addVelocity(cpos, velocityInc * dir);
				}
			}
		}
	}

	if (enableTracers) {
		fluid.addTracer(pos, tracerColor);
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

			// get color from the rgb density maps in the fluid sim
			glm::vec3 color = fluidBox.getColorAtPos(glm::vec2(x,y));
			data[index + 2] = color.x;
			data[index + 3] = color.y;
			data[index + 4] = color.z;

			// override color if a tracer is there
			if (enableTracers && tracerMap[y][x] != nullptr) {
				data[index + 2] = tracerMap[y][x]->color.x;
				data[index + 3] = tracerMap[y][x]->color.y;
				data[index + 4] = tracerMap[y][x]->color.z;
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
		}
	}
}

void updateBuffers(RenderObject* renderObject) {
	glBindVertexArray(renderObject->VAO);

	glBindBuffer(GL_ARRAY_BUFFER, renderObject->VBO);
	glBufferData(GL_ARRAY_BUFFER, ((resolution * resolution - 4 - resolution * 4) * (2 + 3))*sizeof(float), renderObject->data, GL_STATIC_DRAW);

	// position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	// color
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));

	glBindVertexArray(0);
}

void incrementColorIndex() {
	colorIndex = (colorIndex + colorInc) % colorSpectSize;
}

glm::vec2 getScalingVec() {
	return glm::vec2(float(resolution) / SCR_WIDTH, float(resolution) / SCR_HEIGHT);
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

// command methods
string enterCommand() {
	string command = "";

	std::getline(std::cin, command);

	return command;
}

std::vector<string> seperateStringBySpaces(string str) {
	std::vector<string> list = std::vector<string>();

	int startIndex = 0;
	bool trackingString = false;
	for (int i = 0; i < str.length(); i++) {
		if (str[i] != ' ') {
			if (!trackingString) {
				startIndex = i;
				trackingString = true;
			}
		}
		else {
			if (trackingString) {
				list.push_back(str.substr(startIndex, i - startIndex));
				trackingString = false;
			}
		}
	}

	if (trackingString) {
		list.push_back(str.substr(startIndex, str.length() - startIndex));
	}

	return list;
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