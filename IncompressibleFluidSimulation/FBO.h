#pragma once

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

class FBO {
public:
	static int idCount;
	int id;

	int width, height;

	unsigned int fbo;
	unsigned int rbo;
	unsigned int texture;

	FBO(int width, int height);

	unsigned int& createFBO(int width, int height);

	void clear(glm::vec3 color = glm::vec3(0));

	void bind();

	void unbind();

	void useTex(int pos = 0);

	unsigned int& getFBO();
	unsigned int& getTex();
};