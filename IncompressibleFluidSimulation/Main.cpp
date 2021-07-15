#include <iostream>

// utility
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "FluidBox.h"

using namespace std;

int main() {
	FluidBox fluid = FluidBox(100, 1, 1, 0.1);

	fluid.update();

	return 0;
}