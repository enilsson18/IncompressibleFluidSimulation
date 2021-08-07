#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "RenderObject.h"

RenderObject::RenderObject() {
	data = nullptr;

	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);
}

void RenderObject::allocateMemory(int size) {
	if (data != nullptr) {
		delete[] data;
	}

	data = new float[size];
}