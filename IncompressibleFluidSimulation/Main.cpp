#include <iostream>

// utility
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <shader.h>

#include <tuple>
#include <windows.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "FluidBox.h"

using namespace std;

// settings
int resolution = 300;

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
// functions
tuple<unsigned int, unsigned int> findWindowDims(float relativeScreenSize = 0.85, float aspectRatio = 1);
void updateData(FluidBox &fluidBox, float* data);
void updateBuffers(RenderObject* renderObject);

int main() {
	int gameOver = false;

	FluidBox fluid = FluidBox(resolution, 0.0000001, 0.2, 0);

	// init graphics stuff
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	unsigned int width;
	unsigned int height;

	std::tie(width, height) = findWindowDims();

	width = 300;
	height = 300;

	// glfw window creation
	GLFWwindow* window = glfwCreateWindow(width, height, "Fluid Sim", NULL, NULL);

	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
	}
	glfwMakeContextCurrent(window);

	// make callbacks
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	RenderObject* renderFluid = new RenderObject();
	renderFluid->shader = Shader("resources/shaders/point_render.vs", "resources/shaders/point_render.fs");
	renderFluid->allocateMemory(resolution * resolution * (2 + 3) - 4 - resolution * 4);

	updateData(fluid, renderFluid->data);
	updateBuffers(renderFluid);

	for (int y = 1; y < fluid.size - 1; y++) {
		for (int x = 1; x < fluid.size - 1; x++) {
			fluid.addDensity(glm::vec2(x, y), 100);
		}
	}

	//fluid.addVelocity(glm::vec2(resolution / 2.0f), glm::vec2(0.1f, 0));

	std::cout << "entering main loop" << sizeof(*renderFluid->data) / sizeof(float)<< std::endl;

	while (!glfwWindowShouldClose(window)) {
		// process input
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}

		// update frame
		fluid.update();
		updateData(fluid, renderFluid->data);
		updateBuffers(renderFluid);

		//draw
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderFluid->shader.use();
		glBindVertexArray(renderFluid->VAO);
		glDrawArrays(GL_POINTS, 0, resolution * resolution);

		// update view
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwTerminate();
	delete[] renderFluid->data;

	return 0;
}

void updateData(FluidBox &fluidBox, float* data) {
	int index = 0;
	for (int y = 1; y < fluidBox.size-1; y++) {
		for (int x = 1; x < fluidBox.size-1; x++) {
			data[index] = float(x) / fluidBox.size;
			data[index + 1] = float(y) / fluidBox.size;
			data[index + 2] = 0;
			data[index + 3] = 0;
			data[index + 4] = 0;
			//data[index + 2] = fluidBox.density[y][x] / 255.0f;
			//data[index + 3] = fluidBox.density[y][x] / 255.0f;
			//data[index + 4] = fluidBox.density[y][x] / 255.0f;
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

tuple<unsigned int, unsigned int> findWindowDims(float relativeScreenSize, float aspectRatio) {
	// set window size to max while also maintaining size ratio
	RECT rect;
	GetClientRect(GetDesktopWindow(), &rect);

	unsigned int SCR_WIDTH = (rect.right - rect.left) * relativeScreenSize;
	unsigned int SCR_HEIGHT = (rect.bottom - rect.top) * relativeScreenSize;

	if (SCR_WIDTH / SCR_HEIGHT < aspectRatio) {
		// base the size off the width
		SCR_HEIGHT = SCR_WIDTH * (1 / aspectRatio);
	}
	if (SCR_HEIGHT / SCR_WIDTH > aspectRatio) {
		// base the size off the height
		SCR_WIDTH = SCR_HEIGHT * (aspectRatio);
	}

	return tuple<unsigned int, unsigned int>{SCR_WIDTH, SCR_HEIGHT};
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// std::cout << "Failed to create GLFW window" << std::endl;
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on some displays

	glViewport(0, 0, width, height);
}