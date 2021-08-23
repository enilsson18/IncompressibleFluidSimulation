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
#include "RenderObject.h"
#include "BlurGL.h"
#include "Quad.h"

using namespace std;

// settings
int resolution = 150;
double fps = 60;

// control settings
bool enableTracers = false;
bool enableColor = true;
bool enableBlur = false;
int blurIterations = 10;

string commandToRead;
std::atomic<bool> enteredCommand;

// status vars
int SCR_HEIGHT;
int SCR_WIDTH;


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

enum ControlMode { NONE = 0, DIRECTIONAL = 1, MOUSE_SWIPE = 2 };

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

 // commands
string enterCommand();
std::vector<string> seperateStringBySpaces(string str);
void printProcessCommandResult(bool result);
bool processCommand(string command);

// methods
tuple<unsigned int, unsigned int> findWindowDims(float relativeScreenSize = 0.85, float aspectRatio = 1);
void setupBlurFBO();
void updateData(FluidBox &fluidBox, float* data);
void updateBuffers(RenderObject* renderObject);
void processControls(GLFWwindow* window, FluidBox& fluid, ControlMode& controlMode);
void updateForces(FluidBox& fluid);
void incrementColorIndex();
glm::vec2 getScalingVec();
glm::vec3 getColorSpect(float n, float m);
bool constrain(glm::vec2& vec, float min, float max);
void constrain(float &num, float min, float max);

// Control structs
MouseData mouse;

// global stuff
// main vars
GLFWwindow* window;
FluidBox* fluid;
RenderObject* renderFluid;

Shader renderToQuad;
Shader textoQuad;

// fbo to give blur
BlurGL* blur;
unsigned int toBlurFBO;
unsigned int toBlur;

// control vars
ControlMode controlMode;
bool freeze;

FPSCounter timer;

// color stuff
int colorIndex;
int colorInc;
int colorSpectSize;

glm::vec3 defaultColor;

glm::vec3 tracerColor;
int tracerRadius;

// key trackers
bool fPressed;

void setup() {
	controlMode = ControlMode::MOUSE_SWIPE;
	freeze = false;

	colorIndex = 0;
	colorInc = 1;
	colorSpectSize = 5;

	defaultColor = glm::vec3(255);

	tracerColor = glm::vec3(defaultColor);
	tracerRadius = 0;

	mouse = MouseData();

	commandToRead = "";
	enteredCommand.store(false);

	fPressed = false;

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

	// init fluid (since it uses the glfw, it must be made after the glfw instance)
	fluid = new FluidBox(resolution, 0.0f, 0.0000001f, 0.4f);

	// main graphics setup
	renderToQuad = Shader("resources/shaders/render_quad.vs", "resources/shaders/render_quad.fs");

	blur = new BlurGL(SCR_WIDTH, SCR_HEIGHT);
	setupBlurFBO();

	// setup fluid render stuff
	renderFluid = new RenderObject();
	renderFluid->shader = Shader("resources/shaders/point_render.vs", "resources/shaders/point_render.fs", "resources/shaders/point_render.gs");
	renderFluid->allocateMemory((resolution * resolution) * (2 + 3));

	updateData(*fluid, renderFluid->data);
	updateBuffers(renderFluid);
}

void setupBlurFBO() {
	// clear just incase this is a reinitialization
	glDeleteFramebuffers(1, &toBlurFBO);
	glDeleteTextures(1, &toBlur);

	//x values
	//make the shadow buffer and bind it to quad fbo
	glGenFramebuffers(1, &toBlurFBO);

	//create an image representing base depth buffer
	glGenTextures(1, &toBlur);
	glBindTexture(GL_TEXTURE_2D, toBlur);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//bind the buffer
	glBindFramebuffer(GL_FRAMEBUFFER, toBlurFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, toBlur, 0);
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void draw() {

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderToQuad.use();
	fluid->density->useTex();

	Quad::render();
}

void drawToBlur() {
	// check if fbo needs to be updated based on the un-updated BlurGL parameters
	if (blur->isSizeInvalid(SCR_WIDTH, SCR_HEIGHT)) {
		setupBlurFBO();
	}

	// draw original output
	glBindFramebuffer(GL_FRAMEBUFFER, toBlurFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	draw();

	// blur
	unsigned int blurredOutput = blur->process(SCR_WIDTH, SCR_HEIGHT, toBlur, blurIterations);
	
	// render blurred output
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderToQuad.use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, blurredOutput);

	Quad::render();
}

void updateFrame(FPSCounter& timer) {
	timer.start();

	processControls(window, *fluid, controlMode);

	// update frame
	if (!freeze) {
		//fluid->update();
		fluid->fadeDensity(0.05f, 0, 255);
	}

	//updateData(*fluid, renderFluid->data);
	//updateBuffers(renderFluid);

	//draw
	if (enableBlur) {
		drawToBlur();
	}
	else {
		draw();
	}

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
		"get res" << std::endl <<
		"get dt" << std::endl <<
		"get visc" << std::endl <<
		"get diff" << std::endl <<
		"get iter" << std::endl <<
		"get blur" << std::endl <<
		"set tracers enabled" << std::endl <<
		"set tracers disabled" << std::endl <<
		"set colors enabled" << std::endl <<
		"set colors disabled" << std::endl <<
		"set blur enabled" << std::endl <<
		"set blur disabled" << std::endl <<
		"set res #" << std::endl <<
		"set dt #.#" << std::endl <<
		"set visc #.#" << std::endl <<
		"set diff #.#" << std::endl <<
		"set iter #" << std::endl <<
		"set blur #" << std::endl <<
		"freeze velocity" << std::endl <<
		"unfreeze velocity" << std::endl;
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

			if (list[1] == "blur") {
				if (list.size() > 2) {
					if (list[2] == "enabled") {
						enableBlur = true;
						return true;
					}
					if (list[2] == "disabled") {
						enableBlur = false;
						return true;
					}
				}
			}

			if (list[1] == "resolution" || list[1] == "res") {
				if (list.size() > 2) {
					float num;
					try {
						num = std::stoi(list[2]);
					}
					catch (std::invalid_argument err) {
						return false;
					}

					constrain(num, 10, SCR_HEIGHT - 1);

					resolution = num;
					fluid->resetSize(resolution);

					renderFluid->allocateMemory((resolution * resolution) * (2 + 3));

					return true;
				}
			}

			if (list[1] == "dt") {
				if (list.size() > 2) {
					float num;
					try {
						num = std::stof(list[2]);
					}
					catch (std::invalid_argument err) {
						return false;
					}

					fluid->dt = num;

					return true;
				}
			}

			if (list[1] == "viscosity" || list[1] == "visc") {
				if (list.size() > 2) {
					float num;
					try {
						num = std::stof(list[2]);
					}
					catch (std::invalid_argument err) {
						return false;
					}

					fluid->visc = num;

					return true;
				}
			}

			if (list[1] == "diffusion" || list[1] == "diff") {
				if (list.size() > 2) {
					float num;
					try {
						num = std::stof(list[2]);
					}
					catch (std::invalid_argument err) {
						return false;
					}

					fluid->diff = num;

					return true;
				}
			}

			if (list[1] == "div_iter" || list[1] == "iter") {
				if (list.size() > 2) {
					float num;
					try {
						num = std::stoi(list[2]);
					}
					catch (std::invalid_argument err) {
						return false;
					}

					fluid->divIter = num;

					return true;
				}
			}

			if (list[1] == "blur") {
				if (list.size() > 2) {
					float num;
					try {
						num = std::stoi(list[2]);
					}
					catch (std::invalid_argument err) {
						return false;
					}

					blurIterations = num;

					return true;
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

			if (list[1] == "resolution" || list[1] == "res") {
				std::cout << "Resolution: " << resolution << std::endl;
				return true;
			}

			if (list[1] == "dt") {
				std::cout << "dt: " << fluid->dt << std::endl;
				return true;
			}

			if (list[1] == "viscosity" || list[1] == "visc") {
				std::cout << "Viscosity: " << fluid->dt << std::endl;
				return true;
			}

			if (list[1] == "diffusion" || list[1] == "diff") {
				std::cout << "Diffusion: " << fluid->diff << std::endl;
				return true;
			}

			if (list[1] == "div_iter" || list[1] == "iter") {
				std::cout << "Divergence Iterations: " << fluid->divIter << std::endl;
				return true;
			}

			if (list[1] == "blur") {
				std::cout << "Blur Iterations: " << blurIterations << std::endl;
				return true;
			}
		}
	}

	if (list[0] == "freeze") {
		if (list.size() > 1) {
			if (list[1] == "velocity") {
				fluid->freezeVelocity();
				return true;
			}
		}
	}

	if (list[0] == "unfreeze") {
		if (list.size() > 1) {
			if (list[1] == "velocity") {
				fluid->unfreezeVelocity();
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

	// freeze velocity
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		if (fPressed == false) {
			if (fluid.getFreezeVelocity()) {
				fluid.unfreezeVelocity();
			}
			else {
				fluid.freezeVelocity();
			}
		}

		fPressed = true;
	}
	else {
		fPressed = false;
	}

	// clear screen controls
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
		fluid.clear();
	}

	// enter commands
	if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_PRESS) {
		//string command = enterCommand();

		//printProcessCommandResult(processCommand(command));
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
	//float densityMultiplier = 0.05f;
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

void addDirectionalFluid(FluidBox& fluid, int brushSize, float densityInc, float velocityInc, glm::vec2 pos, glm::vec2 dir, glm::vec3 color) {
	// create limits for the increments to prevent too much velocity being added (can make divergence unsolvable) and also limits the density
	constrain(densityInc, 0, 50);
	constrain(velocityInc, 0, 0.15f);

	dir = glm::normalize(dir);

	if (!enableColor) {
		color = glm::vec3(defaultColor);
	}

	fluid.addDensity(pos, densityInc * (float(std::rand()) / INT_MAX + 0.5f), color, brushSize);
	if (!fluid.getFreezeVelocity()) {
		fluid.addVelocity(pos, velocityInc * dir, brushSize);
	}


	if (enableTracers) {
		fluid.addTracer(pos, tracerColor);
	}
}

void updateData(FluidBox &fluidBox, float* data) {
	int index = 0;

	for (int y = 0; y < fluidBox.size; y++) {
		for (int x = 0; x < fluidBox.size; x++) {
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

	// override color if a tracer is there
	if (enableTracers) {
		std::vector<Tracer> tracers = fluidBox.getTracers();

		for (int i = 0; i < tracers.size(); i++) {
			int x = tracers[i].pos.x;
			int y = tracers[i].pos.y;

			for (int iy = -tracerRadius; iy <= tracerRadius; iy++) {
				for (int ix = -tracerRadius; ix <= tracerRadius; ix++) {
					int newX = x + ix;
					int newY = y + iy;

					if (newX >= 0 && newX < resolution && newY >= 0 && newY < resolution) {
						float length = glm::length(glm::vec2(ix, iy));

						if (length <= tracerRadius) {
							int index = (newY * (fluidBox.size) + newX) * 5;

							float power = length / 2.0f;

							if (power == 0) {
								power = 1;
							}

							data[index + 2] = tracers[i].color.x * power;
							data[index + 3] = tracers[i].color.y * power;
							data[index + 4] = tracers[i].color.z * power;
						}
					}
				}
			}
		}
	}
}

void updateBuffers(RenderObject* renderObject) {
	glBindVertexArray(renderObject->VAO);

	glBindBuffer(GL_ARRAY_BUFFER, renderObject->VBO);
	glBufferData(GL_ARRAY_BUFFER, ((resolution * resolution) * (2 + 3))*sizeof(float), renderObject->data, GL_STATIC_DRAW);

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