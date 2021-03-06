#include "BlurGL.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <shader.h>

#include "RenderObject.h"
#include "Quad.h"

BlurGL::BlurGL()
{
	shader = Shader("resources/shaders/gausian_blur.vs", "resources/shaders/gausian_blur.fs");
}

BlurGL::BlurGL(int width, int height)
{
	shader = Shader("resources/shaders/gausian_blur.vs", "resources/shaders/gausian_blur.fs");

	setup(width, height);
}

void BlurGL::setup(int width, int height) {
	this->width = width;
	this->height = height;

	// clear just incase this is a reinitialization
	glDeleteFramebuffers(1, &FBOX);
	glDeleteFramebuffers(1, &FBOY);
	glDeleteTextures(1, &outX);
	glDeleteTextures(1, &outY);

	//x values
	//make the shadow buffer and bind it to quad fbo
	glGenFramebuffers(1, &FBOX);

	//create an image representing base depth buffer
	glGenTextures(1, &outX);
	glBindTexture(GL_TEXTURE_2D, outX);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, NULL);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//bind the buffer
	glBindFramebuffer(GL_FRAMEBUFFER, FBOX);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outX, 0);
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//y values
	//make the shadow buffer and bind it to quad fbo
	glGenFramebuffers(1, &FBOY);

	//create an image representing base depth buffer
	glGenTextures(1, &outY);
	glBindTexture(GL_TEXTURE_2D, outY);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, NULL);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//bind the buffer
	glBindFramebuffer(GL_FRAMEBUFFER, FBOY);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outY, 0);
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int &BlurGL::process(int width, int height, unsigned int &inputTex,int blurIterations)
{
	// stop if no more 
	if (blurIterations < 1) { return outY; }

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// check if the window size has changed or this is the first iteration
	if (isSizeInvalid(width, height)) {
		setup(width, height);
	}

	//enter the gausian blur buffer phase
	//render the current information to a quad and then send that data to the shader
	//x axis
	glBindFramebuffer(GL_FRAMEBUFFER, FBOX);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader.use();
	shader.setInt("stage", 0);
	shader.setFloat("textureWidth", width);
	shader.setFloat("textureHeight", height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inputTex);

	Quad::render();

	//y axis
	glBindFramebuffer(GL_FRAMEBUFFER, FBOY);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader.use();
	shader.setInt("stage", 1);
	shader.setFloat("textureWidth", width);
	shader.setFloat("textureHeight", height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, outX);

	Quad::render();

	// reset buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// process more iterations if specified (if blurIterations - 1 is 0 or less then it will just return outY)
	return process(this->width, this->height, outY, blurIterations - 1);
}

unsigned int &BlurGL::getBlur() {
	return outY;
}

bool BlurGL::isSizeInvalid(int width, int height) {
	if (this->width != width || this->height != height) {
		return true;
	}

	return false;
}
