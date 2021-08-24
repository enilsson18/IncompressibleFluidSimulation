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
	// Enter True for detailed to increase the rasterization
	static void render();
	// Follow this pattern for vertices
	// positions        // texture Coords
	// -1.0f, 1.0f, 0.0f, 1.0f, // top left
	// -1.0f, -1.0f, 0.0f, 0.0f, // bottom left
	//	1.0f, 1.0f, 1.0f, 1.0f, // top right
	//	1.0f, -1.0f, 1.0f, 0.0f // bottom right
	// Enter True for detailed to increase the rasterization
	static void customRender(float* vertices);
};
