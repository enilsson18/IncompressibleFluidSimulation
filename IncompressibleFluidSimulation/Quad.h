#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <img/stb_image.h>
//idk but manually importing fixes the problem
//#include <img/ImageLoader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <shader.h>

#include <vector>

#include <string>
#include <iostream>

class Quad {
private:
	static float defaultVertices[16];
public:
	static void render();
	static void customRender(float* vertices);
};
