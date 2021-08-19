#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class FBO {
public:
	static int idCount;
	int id;

	int width, height;

	unsigned int fbo;
	unsigned int texture;

	FBO(int width, int height);

	unsigned int& createFBO(int width, int height);

	void bind();

	void unbind();

	unsigned int& getFBO();
	unsigned int& getTex();
};