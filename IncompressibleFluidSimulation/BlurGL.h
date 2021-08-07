#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <img/stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <shader.h>

#include "RenderObject.h"

class BlurGL {
public:
	Shader shader;
	unsigned int FBOX;
	unsigned int FBOY;
	unsigned int outX;
	unsigned int outY;

	int width;
	int height;

	float strength;

	RenderObject obj;

	BlurGL();
	void setup(int width, int height);
	unsigned int process(int width, int height, unsigned int inputTex);

	unsigned int getBlur();
};