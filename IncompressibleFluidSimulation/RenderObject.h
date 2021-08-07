#pragma once

#include <shader.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class RenderObject {
public:
	Shader shader;
	unsigned int VAO;
	unsigned int VBO;

	float* data;

	RenderObject();

	void allocateMemory(int size);
};